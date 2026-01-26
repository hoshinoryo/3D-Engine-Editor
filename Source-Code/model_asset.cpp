#include <algorithm>
#include <assert.h>

#include "model_asset.h"
#include "WICTextureLoader11.h"
#include "direct3d.h"
#include "default3Dmaterial.h"
#include "skeleton_util.h"
#include "axis_util.h"
#include "texture.h"

using namespace DirectX;

static const int MAX_BONES = 256;

static Texture g_TextureWhite;
static Texture g_NormalFlat;
static bool g_defaultTexReady = false;

// ---- Function Tool ----
static std::wstring Utf8ToWstring(const std::string& s);
static void AssignUVForMesh(Vertex3d* vertex, const aiMesh* mesh);
static void LoadAllModelTextures(ModelAsset* asset, const std::string& directory);
static void ApplySkinWeightToVertices(
	Vertex3d* vertices,
	const aiMesh* mesh,
	const std::unordered_map<std::string, int>& boneNameToIndex
);
static AABB ComputeLocalAABB(const aiMesh* mesh);
static void EnsureDefaultTextures();

// Fbx model file load
ModelAsset* ModelAsset_Load(const char* filename, bool yUp, float scale)
{
	EnsureDefaultTextures();

	ModelAsset* asset = new ModelAsset();
	asset->importScale = scale;
	asset->sourceYup = yUp;

	const std::string modelPath(filename);

	// ---- Assimp import setting ----
	asset->aiScene = aiImportFile(
		filename,
		aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded
	);
	assert(asset->aiScene);

	SkeletonUtil::BuildBoneNameToIndexTable(asset->aiScene, asset->boneNameToIndex);

	const XMMATRIX axisFix = GetAxisConversion(UpFromBool(asset->sourceYup), UpAxis::Y_Up);
	const XMMATRIX importScaleM = XMMatrixScaling(asset->importScale, asset->importScale, asset->importScale);
	asset->importFix = importScaleM * axisFix; // import fix only do once

	asset->meshes.resize(asset->aiScene->mNumMeshes);

	for (unsigned int m = 0; m < asset->aiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = asset->aiScene->mMeshes[m];
		MeshAsset& out = asset->meshes[m];

		out.skinned = (mesh->mNumBones > 0);
		out.materialIndex = mesh->mMaterialIndex;
		out.localAABB = ComputeLocalAABB(mesh);

		// Vertex buffer
		Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			vertex[v].position = XMFLOAT3{ mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z }; // position
			vertex[v].normal   = XMFLOAT3{ mesh->mNormals[v].x,  mesh->mNormals[v].y,  mesh->mNormals[v].z };  // normal
			vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // vertex color

			// Tangent
			if (mesh->mTangents)
			{
				vertex[v].tangent = XMFLOAT3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z };
			}
			else
			{
				vertex[v].tangent = { 1, 0, 0 };
			}
		}

		// Skin weight
		ApplySkinWeightToVertices(vertex, mesh, asset->boneNameToIndex);

		// UV
		AssignUVForMesh(vertex, mesh);

		D3D11_BUFFER_DESC vbd;
		ZeroMemory(&vbd, sizeof(vbd));
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.ByteWidth = UINT(sizeof(Vertex3d) * mesh->mNumVertices);
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA vsd;
		ZeroMemory(&vsd, sizeof(vsd));
		vsd.pSysMem = vertex;

		Direct3D_GetDevice()->CreateBuffer(&vbd, &vsd, &out.vertexBuffer);

		delete[] vertex;

		// Index buffer
		unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			const aiFace* face = &mesh->mFaces[f];
			assert(face->mNumIndices == 3);

			index[f * 3 + 0] = face->mIndices[0];
			index[f * 3 + 1] = face->mIndices[1];
			index[f * 3 + 2] = face->mIndices[2];
		}

		D3D11_BUFFER_DESC ibd;
		ZeroMemory(&ibd, sizeof(ibd));
		ibd.Usage = D3D11_USAGE_DEFAULT;
		ibd.ByteWidth = UINT(sizeof(unsigned int) * mesh->mNumFaces * 3);
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA isd;
		ZeroMemory(&isd, sizeof(isd));
		isd.pSysMem = index;

		Direct3D_GetDevice()->CreateBuffer(&ibd, &isd, &out.indexBuffer);

		delete[] index;

		out.indexCount = mesh->mNumFaces * 3;
	}

	// Model file path analyzation
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory = (pos != std::string::npos) ? modelPath.substr(0, pos) : "";

	LoadAllModelTextures(asset, directory);

	// ---- Material Building ----
	if (asset->aiScene->mNumMaterials > 0)
	{
		asset->materials.resize(asset->aiScene->mNumMaterials, nullptr);

		for (unsigned int i = 0; i < asset->aiScene->mNumMaterials; ++i)
		{
			aiMaterial* aimat = asset->aiScene->mMaterials[i];
			Default3DMaterial* mat = new Default3DMaterial();

			aiString aiName;

			// Set material name
			if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, aiName) && aiName.length > 0)
			{
				mat->SetName(aiName.C_Str());
			}
			else
			{
				char buf[64];
				sprintf_s(buf, "Material %u", i);
				mat->SetName(buf);
			}

			// Diffuse color
			aiColor3D diffuse(1.0f, 1.0f, 1.0f);
			if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{
				mat->SetBaseColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });
			}

			// Specular color
			aiColor3D specular(1.0f, 1.0f, 1.0f);
			if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, specular))
			{
				mat->SetSpecularColor({ specular.r, specular.g, specular.b, 1.0f });
			}

			// Shininess -> SpecularPower
			float shininess = 0.0f;
			if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS, shininess))
			{
				if (shininess > 0.0f)
					mat->SetSpecularPower(shininess);
			}

			// Diffuse map path
			aiString diffuseName;
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName))
			{
				if (diffuseName.length != 0)
				{
					std::string fullpath = directory.empty()
						? std::string(diffuseName.C_Str())
						: (directory + "/" + diffuseName.C_Str());
					mat->SetDiffuseMapPath(fullpath);
				}
			}

			// Normal map path (NORMALS or HEIGHT)
			aiString normalName;
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &normalName) ||
				AI_SUCCESS == aimat->GetTexture(aiTextureType_HEIGHT, 0, &normalName))
			{
				if (normalName.length != 0)
				{
					std::string fullpath = directory.empty()
						? std::string(normalName.C_Str())
						: (directory + "/" + normalName.C_Str());
					mat->SetNormalMapPath(fullpath);
				}
			}

			// Specular map path
			aiString specName;
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SPECULAR, 0, &specName))
			{
				if (specName.length != 0)
				{
					std::string fullpath = directory.empty()
						? std::string(specName.C_Str())
						: (directory + "/" + specName.C_Str());
					mat->SetSpecularMapPath(fullpath);
				}
			}

			asset->materials[i] = mat;
			Defaul3DMaterial_Register(mat);
		}
	}

	return asset;
}

void ModelAsset_Release(ModelAsset* asset)
{
	if (!asset) return;

	for (auto& m : asset->meshes)
	{
		if (m.vertexBuffer) m.vertexBuffer->Release();
		if (m.indexBuffer) m.indexBuffer->Release();
	}
	asset->meshes.clear();

	for (auto& pair : asset->textures)
	{
		if (pair.second) pair.second->Release();
	}
	asset->textures.clear();

	for (Default3DMaterial* mat : asset->materials)
	{
		if (mat)
		{
			delete mat;
		}
	}
	asset->materials.clear();

	if (asset->aiScene) aiReleaseImport(asset->aiScene);
	delete asset;
}

// utf-8 to wide string
std::wstring Utf8ToWstring(const std::string& s)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring ws(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
	if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
	return ws;
}

// Assign uv for mesh
static void AssignUVForMesh(Vertex3d* vertex, const aiMesh* mesh)
{
	if (mesh->HasTextureCoords(0))
	{
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			vertex[v].texcoord = XMFLOAT2{ mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
		}
	}
	else
	{
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			vertex[v].texcoord = XMFLOAT2{ 0.0f, 0.0f };
		}
	}
}

// Load model with textures
static void LoadAllModelTextures(ModelAsset* asset, const std::string& directory)
{
	// DIFFUSE MAP
	// テクスチャが内包されている場合
	for (unsigned int i = 0; i < asset->aiScene->mNumTextures; ++i)
	{
		aiTexture* aitexture = asset->aiScene->mTextures[i];

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		HRESULT hr = CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			reinterpret_cast<const uint8_t*>(aitexture->pcData),
			static_cast<size_t>(aitexture->mWidth),
			&resource,
			&texture
		);
		if (SUCCEEDED(hr) && texture)
		{
			resource->Release();
			asset->textures[aitexture->mFilename.data] = texture;
		}
	}

	// テクスチャがFBXとは別に用意されている場合
	for (unsigned int m = 0; m < asset->aiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = asset->aiScene->mMaterials[m];
		aiString diffuseName;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName))
		{
			continue;
		} // with no diffuse texture

		if (asset->textures.count(diffuseName.C_Str()))
		{
			continue;
		}

		std::string fullpath = directory.empty()
			? std::string(diffuseName.C_Str())
			: (directory + "/" + diffuseName.C_Str());

		std::wstring wpath = Utf8ToWstring(fullpath);

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		HRESULT hr = CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			wpath.c_str(),
			&resource,
			&texture
		);
		if (SUCCEEDED(hr) && texture)
		{
			resource->Release();
			asset->textures[diffuseName.C_Str()] = texture;
		}
	}

	// NORMAL MAP
	for (unsigned int m = 0; m < asset->aiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = asset->aiScene->mMaterials[m];
		aiString normalName;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_NORMALS, 0, &normalName))
		{
			if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &normalName))
			{
				continue;
			}
		} // with no normal map

		if (asset->textures.count(normalName.C_Str()))
		{
			continue;
		}

		std::string fullpath = directory.empty()
			? std::string(normalName.C_Str())
			: (directory + "/" + normalName.C_Str());

		std::wstring wpath = Utf8ToWstring(fullpath);

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		HRESULT hr = CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			wpath.c_str(),
			&resource,
			&texture
		);
		if (SUCCEEDED(hr) && texture)
		{
			resource->Release();
			asset->textures[normalName.C_Str()] = texture;
		}
	}

	// SPECULAR MAP
	for (unsigned int m = 0; m < asset->aiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = asset->aiScene->mMaterials[m];
		aiString specNAME;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &specNAME))
		{
			continue;
		} // with no normal map

		if (asset->textures.count(specNAME.C_Str()))
		{
			continue;
		}

		std::string fullpath = directory.empty()
			? std::string(specNAME.C_Str())
			: (directory + "/" + specNAME.C_Str());

		std::wstring wpath = Utf8ToWstring(fullpath);

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		HRESULT hr = CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			wpath.c_str(),
			&resource,
			&texture
		);
		if (SUCCEEDED(hr) && texture)
		{
			resource->Release();
			asset->textures[specNAME.C_Str()] = texture;
		}
	}
}

void ApplySkinWeightToVertices(Vertex3d* vertices, const aiMesh* mesh, const std::unordered_map<std::string, int>& boneNameToIndex)
{
	if (!mesh) return;

	const unsigned int vertexCount = mesh->mNumVertices;

	// Initialize all skin weight
	for (unsigned int v = 0; v < vertexCount; ++v)
	{
		for (int i = 0; i < 4; ++i)
		{
			vertices[v].boneIndex[i] = 0;
			vertices[v].boneWeight[i] = 0.0f;
		}
	}

	if (mesh->mNumBones == 0) return;

	// Skin weight
	for (unsigned int b = 0; b < mesh->mNumBones; ++b)
	{
		const aiBone* bone = mesh->mBones[b];
		if (!bone) continue;

		std::string boneName = bone->mName.C_Str();

		auto itIndex = boneNameToIndex.find(boneName);
		if (itIndex == boneNameToIndex.end())
			continue;

		int boneIndex = itIndex->second;

		if (boneIndex < 0 || boneIndex >= MAX_BONES)
			continue;

		for (unsigned int w = 0; w < bone->mNumWeights; ++w)
		{
			const aiVertexWeight& vw = bone->mWeights[w];
			unsigned int vId = vw.mVertexId;
			float weight = vw.mWeight;

			if (vId >= vertexCount) continue;

			Vertex3d& vtx = vertices[vId];

			int slot = -1;
			for (int i = 0; i < 4; ++i)
			{
				if (vtx.boneWeight[i] == 0.0f)
				{
					slot = i;
					break;
				}
			}

			if (slot == -1)
			{
				int smallest = 0;

				for (int i = 1; i < 4; ++i)
				{
					if (vtx.boneWeight[i] < vtx.boneWeight[smallest])
						smallest = i;
				}
				slot = smallest;
			}

			vtx.boneIndex[slot] = static_cast<UINT>(boneIndex);
			vtx.boneWeight[slot] = weight;
		}
	}

	// Normalize skin weight
	for (unsigned int v = 0; v < vertexCount; ++v)
	{
		float sum =
			vertices[v].boneWeight[0] +
			vertices[v].boneWeight[1] +
			vertices[v].boneWeight[2] +
			vertices[v].boneWeight[3];

		if (sum > 0.0f)
		{
			float inv = 1.0f / sum;
			for (int i = 0; i < 4; ++i)
			{
				vertices[v].boneWeight[i] *= inv;
			}
		}
	}
}

static AABB ComputeLocalAABB(const aiMesh* mesh)
{
	XMFLOAT3 min = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (unsigned int v = 0; v < mesh->mNumVertices; v++)
	{
		const aiVector3D& p = mesh->mVertices[v];

		min.x = std::min(min.x, p.x);
		min.y = std::min(min.y, p.y);
		min.z = std::min(min.z, p.z);

		max.x = std::max(max.x, p.x);
		max.y = std::max(max.y, p.y);
		max.z = std::max(max.z, p.z);
	}

	return { min, max };
}

static void EnsureDefaultTextures()
{
	if (g_defaultTexReady) return;
	
	g_TextureWhite.Load(L"resources/white.png");
	g_NormalFlat.Load(L"resources/normal_flat.png");
	g_defaultTexReady = true;
}
