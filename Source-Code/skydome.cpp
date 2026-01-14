/*==============================================================================

   ‹ó‚Ì•`‰æ [skydome.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/21

--------------------------------------------------------------------------------

==============================================================================*/

#include "skydome.h"
#include "model.h"
#include "unlit_shader.h"
#include "direct3d.h"

#include <DirectXMath.h>

using namespace DirectX;

static MODEL* g_pModelSky{nullptr};
static XMFLOAT3 g_Position{};

//static UnlitShader g_SkydomeShader;

void Skydome_Initialize()
{
	//g_SkydomeShader.Initialize();

	g_pModelSky = ModelLoad("resources/sky.fbx", false, 100.0f);
}

void Skydome_Finalize()
{
	ModelRelease(g_pModelSky);

	//g_SkydomeShader.Finalize();
}

void Skydome_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}

void Skydome_Draw()
{
	Direct3D_BeginSkydome();

	ModelUnlitDraw(g_pModelSky, XMMatrixTranslationFromVector(XMLoadFloat3(&g_Position)), { 1.0f, 1.0f, 1.0f, 1.0f });

	Direct3D_EndSkydome();
}
