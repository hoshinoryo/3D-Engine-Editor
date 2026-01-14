/*==============================================================================

   グリッドの表示 [grid.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#include "grid.h"
#include "draw3d.h"

#include <DirectXMath.h>

using namespace DirectX;

static constexpr int GRID_H_COUNT = 20;
static constexpr int GRID_V_COUNT = 20;
static constexpr float GRID_HALF_SIZE = 10.0f;


void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	Draw3d_Initialize(pDevice, pContext);
}

void Grid_Finalize(void)
{
	//SAFE_RELEASE(g_pVertexBuffer);
}

void Grid_Draw(void)
{
	// Grid Line

	XMFLOAT4 gridColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	{
		// 垂直線
		float x = -GRID_HALF_SIZE;
		const float step = (GRID_HALF_SIZE * 2.0f) / GRID_H_COUNT;

		for (int i = 0; i <= GRID_H_COUNT; ++i)
		{
			XMFLOAT3 p0(x, 0.0f, GRID_HALF_SIZE);
			XMFLOAT3 p1(x, 0.0f, -GRID_HALF_SIZE);
			Draw3d_MakeLine(p0, p1, gridColor);
			x += step;
		}
	}
	
	{
		// 水平線
		float z = -GRID_HALF_SIZE;
		const float step = (GRID_HALF_SIZE * 2.0f) / GRID_V_COUNT;

		for (int i = 0; i <= GRID_V_COUNT; ++i)
		{
			XMFLOAT3 p0(GRID_HALF_SIZE, 0.0f, z);
			XMFLOAT3 p1(-GRID_HALF_SIZE, 0.0f, z);
			Draw3d_MakeLine(p0, p1, gridColor);
			z += step;
		}
	}
	
}
