#include "model_renderer.h"
#include "model_asset.h"
#include "direct3d.h"
#include "default3Dshader.h"
#include "unlit_shader.h"
#include "default3Dmaterial.h"
#include "texture.h"

using namespace DirectX;

extern Default3DMaterial g_DefaultSceneMaterial;

static Texture g_TextureWhite;
static Texture g_NormalFlat;

static AABB TransformAABB(const AABB& local, const XMMATRIX& world);

void ModelRenderer_Draw(
	ModelAsset* asset,
	uint32_t meshIndex,
	const XMMATRIX& world,
	const XMFLOAT3& cameraPos
)
{
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

	mat->Apply(shader, cameraPos);

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
	if (!asset) return;
	if (meshIndex >= asset->meshes.size()) return;

	MeshAsset& mesh = asset->meshes[meshIndex];
	if (mesh.skinned) return;

	//if (!Outliner::IsMeshVisible(asset, (int)meshIndex)) return;

	g_DefaultUnlitShader.Begin();

	const XMMATRIX finalWorld = asset->importFix * world; // import fix

	g_DefaultUnlitShader.SetWorldMatrix(finalWorld);
	g_DefaultUnlitShader.SetColor(color);

	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Binding VB and IB
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &mesh.vertexBuffer, &stride, &offset);
	Direct3D_GetContext()->IASetIndexBuffer(mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	Direct3D_GetContext()->DrawIndexed(mesh.indexCount, 0, 0);
}
