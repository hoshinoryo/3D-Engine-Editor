#include <DirectXMath.h>
#include <assert.h>
#include <functional>

#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include "WICTextureLoader11.h"
#include "default3Dshader.h"
#include "unlit_shader.h"
#include "scene_manager.h"
#include "outliner.h"
#include "default3Dmaterial.h"
#include "skeleton_util.h"
#include "axis_util.h"

using namespace DirectX;

// ---- Function Tool ----
static std::wstring Utf8ToWstring(const std::string& s);
static aiMatrix4x4 XMToAi(const XMMATRIX& m);

static void AssignUVForMesh(Vertex3d* vertex, const aiMesh* mesh);
static void LoadAllModelTextures(MODEL* model, const std::string& directory);
static void ApplySkinWeightToVertices(
	Vertex3d* vertices,
	const aiMesh* mesh,
	const std::unordered_map<std::string, int>& boneNameToIndex
);

static AABB ComputeLocalAABB(const MODEL* model);
static AABB TransformAABB(const AABB& local, const XMMATRIX& world);


// -----------------------

static Texture g_TextureWhite;
static Texture g_NormalFlat;

static const int MAX_BONES = 256;

MODEL* ModelLoad( const char *FileName, bool yUp, float scale)
{
	g_TextureWhite.Load(L"resources/white.png");
	g_NormalFlat.Load(L"resources/normal_flat.png");

	MODEL* model = new MODEL;
	model->Scale = scale;
	model->SourceYup = yUp;
	const std::string modelPath(FileName);

	// ---- Assimp import setting ----
	model->AiScene = aiImportFile(
		FileName,
		aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded
	);
	assert(model->AiScene);

	SkeletonUtil::BuildBoneNameToIndexTable(model->AiScene, model->BoneNameToIndex);

	//XMMATRIX axisFix = GetAxisFixMatrixFromSource(model->SourceYup);
	XMMATRIX axisFix = GetAxisConversion(UpFromBool(model->SourceYup), UpAxis::Y_Up);

	model->VertexBuffer = new ID3D11Buffer*[model->AiScene->mNumMeshes];
	model->IndexBuffer  = new ID3D11Buffer*[model->AiScene->mNumMeshes];

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				// Position
				vertex[v].position = XMFLOAT3{
					mesh->mVertices[v].x,
					mesh->mVertices[v].y,
					mesh->mVertices[v].z,
				};

				// Normal
				vertex[v].normal = XMFLOAT3{
					mesh->mNormals[v].x,
					mesh->mNormals[v].y,
					mesh->mNormals[v].z,
				};

				// Vertex color
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

				// Tangent
				if (mesh->mTangents)
				{
					vertex[v].tangent = XMFLOAT3(
						mesh->mTangents[v].x,
						mesh->mTangents[v].y,
						mesh->mTangents[v].z
					);
				}
				else
				{
					XMVECTOR N = XMLoadFloat3(&vertex[v].normal);
					XMVECTOR T = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

					if (fabs(XMVectorGetX(XMVector3Dot(N, T))) > 0.99f)
					{
						T = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
					}
					T = XMVector3Normalize(T - XMVector3Dot(T, N) * N);

					XMStoreFloat3(&vertex[v].tangent, T);
				}
			}

			// Skin weight
			ApplySkinWeightToVertices(vertex, mesh, model->BoneNameToIndex);

			// UV
			AssignUVForMesh(vertex, mesh);

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(Vertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
		}


		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];
				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
		}

	}

	size_t pos = modelPath.find_last_of("/\\");
	std::string directory = (pos != std::string::npos) ? modelPath.substr(0, pos) : "";

	LoadAllModelTextures(model, directory);

	// ---- Material Building ----
	if (model->AiScene->mNumMaterials > 0)
	{
		model->Materials.resize(model->AiScene->mNumMaterials, nullptr);

		for (unsigned int i = 0; i < model->AiScene->mNumMaterials; ++i)
		{
			aiMaterial* aimat = model->AiScene->mMaterials[i];
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

			// Diffuse mao path
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

			model->Materials[i] = mat;
			Defaul3DMaterial_Register(mat);
		}
	}

	// AABB
	model->localAABB = ComputeLocalAABB(model);

	return model;
}


void ModelRelease(MODEL* model)
{
	//SceneManager::Unregister(model);

	for (Default3DMaterial* mat : model->Materials)
	{
		if (mat && mat != &g_DefaultSceneMaterial)
		{
			Defaul3DMaterial_Unregister(mat);
			delete mat;
		}
	}
	model->Materials.clear();

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		model->VertexBuffer[m]->Release();
		model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;


	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		pair.second->Release();
	}

	aiReleaseImport(model->AiScene);

	delete model;
}

void ModelDraw(MODEL* model, const XMMATRIX& mtxWorld, const XMFLOAT3& cameraPos)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		const bool isSkinned = (mesh->mNumBones > 0);

		Default3DShader& shader = isSkinned ? g_Default3DshaderSkinned : g_Default3DshaderStatic;
		shader.Begin();

		// ここでaxisFixとscaleを導入
		XMMATRIX axisFix = GetAxisConversion(UpFromBool(model->SourceYup), UpAxis::Y_Up);
		XMMATRIX importScale = XMMatrixScaling(model->Scale, model->Scale, model->Scale);
		XMMATRIX finalWorld = importScale * axisFix * mtxWorld;

		shader.SetWorldMatrix(finalWorld);
		model->worldAABB = TransformAABB(model->localAABB, finalWorld);

		Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (!Outliner::IsMeshVisible(model, (int)m)) continue;

		unsigned int matIndex = mesh->mMaterialIndex;

		// Choose the material for this mesh
		Default3DMaterial* mat = nullptr;
		if (matIndex < model->Materials.size() && model->Materials[matIndex])
		{
			mat = model->Materials[matIndex];
		}
		else
		{
			mat = &g_DefaultSceneMaterial;
		}

		// Apply material
		mat->Apply(shader, cameraPos);

		aiMaterial* aimaterial = model->AiScene->mMaterials[matIndex];

		aiString diffuseTex;
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTex);

		if (diffuseTex.length != 0 && model->Texture.count(diffuseTex.data))
		{
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[diffuseTex.data]);
		}
		else
		{
			g_TextureWhite.SetTexture();
		}

		aiString normalName;
		bool hasNormalDecl =
			(AI_SUCCESS == aimaterial->GetTexture(aiTextureType_NORMALS, 0, &normalName)) ||
			(AI_SUCCESS == aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &normalName));

		if (hasNormalDecl && normalName.length != 0 && model->Texture.count(normalName.data))
		{
			Direct3D_GetContext()->PSSetShaderResources(1, 1, &model->Texture[normalName.data]);
		}
		else
		{
			g_NormalFlat.SetTexture(1);
		}

		aiString specName;
		if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &specName)
			&& specName.length != 0 && model->Texture.count(specName.data))
		{
			Direct3D_GetContext()->PSSetShaderResources(2, 1, &model->Texture[specName.data]);
		}
		else
		{
			g_TextureWhite.SetTexture(2);
		}

		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}

void ModelUnlitDraw(MODEL* model, const XMMATRIX& mtxWorld, const XMFLOAT4& color)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		if (mesh->mNumBones > 0)
			continue;

		if (!Outliner::IsMeshVisible(model, (int)m))
			continue;

		g_DefaultUnlitShader.Begin();

		XMMATRIX axisFix = GetAxisConversion(UpFromBool(model->SourceYup), UpAxis::Y_Up);
		XMMATRIX importScale = XMMatrixScaling(model->Scale, model->Scale, model->Scale);
		XMMATRIX finalWorld = importScale * axisFix * mtxWorld;

		g_DefaultUnlitShader.SetWorldMatrix(finalWorld);

		g_DefaultUnlitShader.SetColor(color);

		Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (!Outliner::IsMeshVisible(model, (int)m)) continue;

		// Bine textures: diffuse only
		unsigned int matIndex = mesh->mMaterialIndex;
		aiMaterial* aimaterial = model->AiScene->mMaterials[matIndex];

		aiString diffuseTex;
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTex);

		if (diffuseTex.length != 0 && model->Texture.count(diffuseTex.data))
		{
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[diffuseTex.data]);
		}
		else
		{
			g_TextureWhite.SetTexture();
		}

		// Unlitではnormal/specは触らない or nullでクリアしておく
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		Direct3D_GetContext()->PSSetShaderResources(1, 1, nullSRV);
		Direct3D_GetContext()->PSSetShaderResources(2, 1, nullSRV);

		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
}

static float ModelGetScale(const MODEL* model)
{
	assert(model);
	return model->Scale;
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

static aiMatrix4x4 XMToAi(const XMMATRIX& m)
{
	XMFLOAT4X4 f;
	XMStoreFloat4x4(&f, m);

	aiMatrix4x4 r;
	r.a1 = f._11; r.b1 = f._12; r.c1 = f._13; r.d1 = f._14;
	r.a2 = f._21; r.b2 = f._22; r.c2 = f._23; r.d2 = f._24;
	r.a3 = f._31; r.b3 = f._32; r.c3 = f._33; r.d3 = f._34;
	r.a4 = f._41; r.b4 = f._42; r.c4 = f._43; r.d4 = f._44;

	return r;
}

// Assign uv for mesh
static void AssignUVForMesh(Vertex3d* vertex, const aiMesh* mesh)
{
	if (mesh->HasTextureCoords(0))
	{
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			vertex[v].texcoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
		}
	}
	else
	{
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			vertex[v].texcoord = XMFLOAT2(0.0f, 0.0f);
		}
	}
}

// Load model with textures
static void LoadAllModelTextures(MODEL* model, const std::string& directory)
{
	// DIFFUSE MAP
	// テクスチャが内包されている場合
	for (unsigned int i = 0; i < model->AiScene->mNumTextures; ++i)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

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
			model->Texture[aitexture->mFilename.data] = texture;
		}
	}

	// テクスチャがFBXとは別に用意されている場合
	for (unsigned int m = 0; m < model->AiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = model->AiScene->mMaterials[m];
		aiString diffuseName;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName))
		{
			continue;
		} // with no diffuse texture

		if (model->Texture.count(diffuseName.C_Str()))
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
			model->Texture[diffuseName.C_Str()] = texture;
		}
	}
	
	// NORMAL MAP
	for (unsigned int m = 0; m < model->AiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = model->AiScene->mMaterials[m];
		aiString normalName;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_NORMALS, 0, &normalName))
		{
			if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &normalName))
			{
				continue;
			}
		} // with no normal map

		if (model->Texture.count(normalName.C_Str()))
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
			model->Texture[normalName.C_Str()] = texture;
		}
	}

	// SPECULAR MAP
	for (unsigned int m = 0; m < model->AiScene->mNumMaterials; ++m)
	{
		aiMaterial* aimaterial = model->AiScene->mMaterials[m];
		aiString specNAME;

		if (AI_SUCCESS != aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &specNAME))
		{
			continue;
		} // with no normal map

		if (model->Texture.count(specNAME.C_Str()))
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
			model->Texture[specNAME.C_Str()] = texture;
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


static AABB ComputeLocalAABB(const MODEL* model)
{
	XMFLOAT3 min = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

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
	}

	return { min, max };
}

static AABB TransformAABB(const AABB& local, const XMMATRIX& world)
{
	const XMFLOAT3& min = local.min;
	const XMFLOAT3& max = local.max;

	XMVECTOR corners[8] =
	{
		XMVectorSet(min.x, min.y, min.z, 1),
		XMVectorSet(max.x, min.y, min.z, 1),
		XMVectorSet(min.x, max.y, min.z, 1),
		XMVectorSet(max.x, max.y, min.z, 1),
		XMVectorSet(min.x, min.y, max.z, 1),
		XMVectorSet(max.x, min.y, max.z, 1),
		XMVectorSet(min.x, max.y, max.z, 1),
		XMVectorSet(max.x, max.y, max.z, 1),
	};

	XMFLOAT3 outMin{  FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 outMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (int i = 0; i < 8; i++)
	{
		XMVECTOR p = XMVector3TransformCoord(corners[i], world);
		XMFLOAT3 pf;
		XMStoreFloat3(&pf, p);

		outMin.x = std::min(outMin.x, pf.x);
		outMin.y = std::min(outMin.y, pf.y);
		outMin.z = std::min(outMin.z, pf.z);

		outMax.x = std::max(outMax.x, pf.x);
		outMax.y = std::max(outMax.y, pf.y);
		outMax.z = std::max(outMax.z, pf.z);
	}

	return { outMin, outMax };
}
