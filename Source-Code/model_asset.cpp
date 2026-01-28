/*==============================================================================

   Model asset import and release [model_asset.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/28
--------------------------------------------------------------------------------

==============================================================================*/

#include <algorithm>
#include <assert.h>
#include <unordered_set>

#include "model_asset.h"
#include "WICTextureLoader11.h"
#include "direct3d.h"
#include "default3Dmaterial.h"
#include "skeleton_util.h"
#include "axis_util.h"
#include "texture.h"

using namespace DirectX;

static const int MAX_BONES = 256;

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

// Path normalization
static std::string NormalizePath(std::string p);
static std::string Basename(std::string p);
static bool IsAbsolutePath(const std::string& p);
static std::string MakeTextureKey(const std::string& directory, const std::string& raw);
static void RegisterTextureAlias(ModelAsset* asset, ID3D11ShaderResourceView* srv, const std::string& key);


// Fbx model file load
ModelAsset* ModelAsset_Load(const char* filename, bool yUp, float scale)
{
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
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName) && diffuseName.length != 0)
			{
				mat->SetDiffuseMapPath(MakeTextureKey(directory, diffuseName.C_Str()));
			}

			// Normal map path (NORMALS or HEIGHT)
			aiString normalName;
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &normalName) ||
				AI_SUCCESS == aimat->GetTexture(aiTextureType_HEIGHT, 0, &normalName) &&
				normalName.length != 0)
			{
				mat->SetNormalMapPath(MakeTextureKey(directory, normalName.C_Str()));
			}

			// Specular map path
			aiString specName;
			if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SPECULAR, 0, &specName) && specName.length != 0)
			{
				mat->SetSpecularMapPath(MakeTextureKey(directory, specName.C_Str()));
			}

			asset->materials[i] = mat;
			Default3DMaterial_Register(mat);
		}
	}

	return asset;
}

void ModelAsset_Release(ModelAsset* asset)
{
	if (!asset) return;

	for (auto& m : asset->meshes)
	{
		if (m.vertexBuffer)
		{
			m.vertexBuffer->Release();
			m.vertexBuffer = nullptr;
		}
		if (m.indexBuffer)
		{
			m.indexBuffer->Release();
			m.indexBuffer = nullptr;
		}
	}
	asset->meshes.clear();

	std::unordered_set<ID3D11ShaderResourceView*> released;
	for (auto& pair : asset->textures)
	{
		ID3D11ShaderResourceView* srv = pair.second;
		if (!srv) continue;
		if (released.insert(srv).second)
		{
			srv->Release();
		}
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
	// テクスチャが内包されている場合内
	for (unsigned int i = 0; i < asset->aiScene->mNumTextures; ++i)
	{
		aiTexture* aitexture = asset->aiScene->mTextures[i];
		if (!aitexture) continue;

		ID3D11ShaderResourceView* texture = nullptr;
		ID3D11Resource* resource = nullptr;

		const size_t bytes = (aitexture->mHeight == 0)
			? static_cast<size_t>(aitexture->mWidth)
			: static_cast<size_t>(aitexture->mWidth) * static_cast<size_t>(aitexture->mHeight) * 4;

		HRESULT hr = CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			reinterpret_cast<const uint8_t*>(aitexture->pcData),
			bytes,
			&resource,
			&texture
		);
		if (SUCCEEDED(hr) && texture)
		{
			resource->Release();

			const std::string starKey = "*" + std::to_string(i);
			//asset->textures[starKey] = texture;
			RegisterTextureAlias(asset, texture, starKey);

			if (aitexture->mFilename.length > 0)
			{
				//asset->textures[aitexture->mFilename.C_Str()] = texture;
				RegisterTextureAlias(asset, texture, aitexture->mFilename.C_Str());
			}			
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

		std::string rawName = diffuseName.C_Str();
		std::string fullKey = MakeTextureKey(directory, rawName);

		std::wstring wpath = Utf8ToWstring(fullKey);

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

			RegisterTextureAlias(asset, texture, fullKey);
			RegisterTextureAlias(asset, texture, rawName);
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

		std::string rawName = normalName.C_Str();
		std::string fullKey = MakeTextureKey(directory, rawName);

		std::wstring wpath = Utf8ToWstring(fullKey);

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

			RegisterTextureAlias(asset, texture, fullKey);
			RegisterTextureAlias(asset, texture, rawName);
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
		} // with no specular map

		if (asset->textures.count(specNAME.C_Str()))
		{
			continue;
		}

		std::string rawName = specNAME.C_Str();
		std::string fullKey = MakeTextureKey(directory, rawName);

		std::wstring wpath = Utf8ToWstring(fullKey);

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

			RegisterTextureAlias(asset, texture, fullKey);
			RegisterTextureAlias(asset, texture, rawName);
		}
	}
}

static std::string NormalizePath(std::string p)
{
	// Unified separator
	for (auto& c : p)
	{
		if (c == '\\') c = '/';
	}

	// Remove "./"
	while (p.rfind("./", 0) == 0)
		p.erase(0, 2);

	// Remove repeated "//"
	for (;;)
	{
		auto pos = p.find("//");
		if (pos == std::string::npos) break;
		p.erase(pos, 1);
	}

	return p;
}

static std::string Basename(std::string p)
{
	p = NormalizePath(std::move(p));
	size_t pos = p.find_last_of('/');
	return (pos == std::string::npos) ? p : p.substr(pos + 1);
}

static bool IsAbsolutePath(const std::string& p)
{
	if (p.size() >= 2 && std::isalpha((unsigned char)p[0]) && p[1] == ':')
		return true;
	if (!p.empty() && (p[0] == '/'))
		return true;
	return false;
}

static std::string MakeTextureKey(const std::string& directory, const std::string& raw)
{
	std::string key = NormalizePath(raw);
	if (key.empty()) return key;

	if (key[0] == '*') return key; // 内包テクスチャ

	if (IsAbsolutePath(key)) return key;

	if (directory.empty()) return key;

	return NormalizePath(directory + "/" + key);
}

static void RegisterTextureAlias(ModelAsset* asset, ID3D11ShaderResourceView* srv, const std::string& key)
{
	if (!srv) return;

	const std::string k = NormalizePath(key);

	if (k.empty()) return;
	asset->textures[k] = srv;

	const std::string base = Basename(k);
	if (!base.empty()) asset->textures[base] = srv;
}

static void ApplySkinWeightToVertices(Vertex3d* vertices, const aiMesh* mesh, const std::unordered_map<std::string, int>& boneNameToIndex)
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
