/*==============================================================================

   Model asset drawing functions [model_renderer.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/28
--------------------------------------------------------------------------------

==============================================================================*/

#include "model_renderer.h"
#include "model_asset.h"
#include "direct3d.h"
#include "default3Dshader.h"
#include "unlit_shader.h"
#include "default3Dmaterial.h"
#include "texture.h"
#include "debug_ostream.h"

using namespace DirectX;

extern Default3DMaterial g_DefaultSceneMaterial;

static Texture g_TextureWhite;
static Texture g_NormalFlat;
static bool g_TexReady = false;

static void BindPS_SRV(UINT slot, ID3D11ShaderResourceView* srv);
static ID3D11ShaderResourceView* FindSRV(ModelAsset* asset, const std::string& key);

//static AABB TransformAABB(const AABB& local, const XMMATRIX& world);


void ModelRenderer_Initialize()
{
	if (g_TexReady) return;

	g_TextureWhite.Load(L"resources/white.png");
	g_NormalFlat.Load(L"resources/normal_flat.png");

	g_TexReady = true;
}

void ModelRenderer_Finalize()
{
	if (!g_TexReady) return;

	g_TextureWhite.Release();
	g_NormalFlat.Release();

	g_TexReady = false;
}

void ModelRenderer_Draw(
	ModelAsset* asset,
	uint32_t meshIndex,
	const XMMATRIX& world,
	const XMFLOAT3& cameraPos
)
{
	ModelRenderer_Initialize();

	if (!asset) return;
	if (meshIndex >= asset->meshes.size()) return;

	MeshAsset& mesh = asset->meshes[meshIndex];

	Default3DShader& shader = mesh.skinned ? g_Default3DshaderSkinned : g_Default3DshaderStatic;
	shader.Begin();

	const XMMATRIX finalWorld = asset->importFix * world; // import fix

	shader.SetWorldMatrix(finalWorld);

	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Choose the material for this mesh
	Default3DMaterial* mat = &g_DefaultSceneMaterial;
	if (mesh.materialIndex < asset->materials.size() && asset->materials[mesh.materialIndex])
	{
		mat = asset->materials[mesh.materialIndex];
	}

	mat->Apply(shader, cameraPos); // bind texture to shader

	// Binding SRV
	ID3D11ShaderResourceView* diffuseSRV  = nullptr;
	ID3D11ShaderResourceView* normalSRV   = nullptr;
	ID3D11ShaderResourceView* specularSRV = nullptr;

	const std::string& diffuseKey  = mat->GetDiffuseMapPath();
	const std::string& normalKey   = mat->GetNormalMapPath();
	const std::string& specularKey = mat->GetSpecularMapPath();

	if (!diffuseKey.empty()) diffuseSRV = FindSRV(asset, diffuseKey);
	if (!normalKey.empty()) normalSRV = FindSRV(asset, normalKey);
	if (!specularKey.empty()) specularSRV = FindSRV(asset, specularKey);

	if (!diffuseSRV) diffuseSRV = g_TextureWhite.GetSRV();
	if (!normalSRV) normalSRV = g_NormalFlat.GetSRV();
	if (!specularSRV) specularSRV = g_TextureWhite.GetSRV();

	BindPS_SRV(0, diffuseSRV);
	BindPS_SRV(1, normalSRV);
	BindPS_SRV(2, specularSRV);

	// Binding VB and IB
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &mesh.vertexBuffer, &stride, &offset);
	Direct3D_GetContext()->IASetIndexBuffer(mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	Direct3D_GetContext()->DrawIndexed(mesh.indexCount, 0, 0);
}

void ModelRenderer_UnlitDraw(
	ModelAsset* asset,
	uint32_t meshIndex,
	const XMMATRIX& world,
	const XMFLOAT4& color
)
{
	ModelRenderer_Initialize();

	if (!asset) return;
	if (meshIndex >= asset->meshes.size()) return;

	MeshAsset& mesh = asset->meshes[meshIndex];
	if (mesh.skinned) return;

	g_DefaultUnlitShader.Begin();

	const XMMATRIX finalWorld = asset->importFix * world; // import fix

	g_DefaultUnlitShader.SetWorldMatrix(finalWorld);
	g_DefaultUnlitShader.SetColor(color);

	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Binding SRV
	ID3D11ShaderResourceView* diffuseSRV = nullptr;

	Default3DMaterial* mat = &g_DefaultSceneMaterial;
	if (mesh.materialIndex < asset->materials.size() && asset->materials[mesh.materialIndex])
		mat = asset->materials[mesh.materialIndex];

	const std::string& diffuseKey = mat->GetDiffuseMapPath();
	if (!diffuseKey.empty()) 
		diffuseSRV = FindSRV(asset, diffuseKey);
	if (!diffuseSRV) 
		diffuseSRV = g_TextureWhite.GetSRV();

	hal::dout << "Unlit diffuseKey = [" << diffuseKey << "]\n";

	BindPS_SRV(0, diffuseSRV);

	// Binding VB and IB
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &mesh.vertexBuffer, &stride, &offset);
	Direct3D_GetContext()->IASetIndexBuffer(mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	Direct3D_GetContext()->DrawIndexed(mesh.indexCount, 0, 0);
}

static void BindPS_SRV(UINT slot, ID3D11ShaderResourceView* srv)
{
	Direct3D_GetContext()->PSSetShaderResources(slot, 1, &srv);
}

static ID3D11ShaderResourceView* FindSRV(ModelAsset* asset, const std::string& key)
{
	if (!asset) return nullptr;
	if (key.empty()) return nullptr;

	auto findExact = [&](const std::string& k) -> ID3D11ShaderResourceView*
		{
			auto it = asset->textures.find(k);
			return (it == asset->textures.end() ? nullptr : it->second);
		};

	if (auto* srv = findExact(key)) return srv;

	std::string norm = key;
	for (auto& c : norm)
	{
		if (c == '\\') c = '/';
	}
	while (norm.rfind("./", 0) == 0)
		norm.erase(0, 2);
	if (auto* srv = findExact(norm)) return srv;

	size_t pos = norm.find_last_of('/');
	std::string base = (pos == std::string::npos) ? norm : norm.substr(pos + 1);
	if (auto* srv = findExact(base)) return srv;

	return nullptr;
}

/*
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
*/