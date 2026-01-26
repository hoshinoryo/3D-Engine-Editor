/*==============================================================================

   ‹ó‚Ì•`‰æ [skydome.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/21

--------------------------------------------------------------------------------

==============================================================================*/

#include "skydome.h"
#include "model_asset.h"
#include "model_renderer.h"
#include "unlit_shader.h"
#include "direct3d.h"

#include <DirectXMath.h>

using namespace DirectX;

static ModelAsset* g_pModelSky{nullptr};
static XMFLOAT3 g_Position{};

//static UnlitShader g_SkydomeShader;

void Skydome_Initialize()
{
	g_pModelSky = ModelAsset_Load("resources/sky.fbx", false, 100.0f);
}

void Skydome_Finalize()
{
	ModelAsset_Release(g_pModelSky);
}

void Skydome_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}

void Skydome_Draw()
{
	Direct3D_BeginSkydome();

	for (uint32_t mi = 0; mi < (uint32_t)g_pModelSky->meshes.size(); ++mi)
	{
		ModelRenderer_UnlitDraw(g_pModelSky, mi, XMMatrixTranslationFromVector(XMLoadFloat3(&g_Position)), { 1.0f, 1.0f, 1.0f, 1.0f });
	}

	Direct3D_EndSkydome();
}
