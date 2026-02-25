/*==============================================================================

   デモシーンのディスプレイ [demo_scene.cpp]
														 Author : Gu Anyi
														 Date   : 2025/12/19

--------------------------------------------------------------------------------

==============================================================================*/

#include <array>

#include "demo_scene.h"
#include "cube.h"
#include "collision.h"
#include "debug_draw_gate.h"
#include "primitive_tool.h"

using namespace DirectX;

static constexpr int X_GROUND_CUBE_COUNT = 17;
static constexpr int Z_GROUND_CUBE_COUNT = 17;
static constexpr int Y_WALL_CUBE_HEIGHT = 4;
static constexpr int WALL_RING_COUNT = 2 * (X_GROUND_CUBE_COUNT + Z_GROUND_CUBE_COUNT + 2);
static constexpr int WALL_CUBE_COUNT = WALL_RING_COUNT * Y_WALL_CUBE_HEIGHT;

static float g_CubeSize = 1.0f;
static std::array<CubeObject, X_GROUND_CUBE_COUNT * Z_GROUND_CUBE_COUNT> g_groundCubes;
static std::array<CubeObject, WALL_CUBE_COUNT> g_wallCubes;

static std::array<CubeObject, X_GROUND_CUBE_COUNT* Z_GROUND_CUBE_COUNT>
GenerateGroundCubePos(float halfExtent, float y = 0.0f);
static std::array<CubeObject, WALL_CUBE_COUNT>
GenerateWallCubePos(float halfExtent, float startY = 0.0f);

void Demo_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	Cube_Initialize(pDevice, pContext, g_CubeSize);

	g_groundCubes = GenerateGroundCubePos(g_CubeSize, -1.0f);
	g_wallCubes = GenerateWallCubePos(g_CubeSize, -1.0f);

	PrimitiveTool::Initialize(g_CubeSize);

	Demo_AddColliders();
}

void Demo_Finalize(void)
{
	PrimitiveTool::Finalize();

	Cube_Finalize();
}

void Demo_Draw()
{
	for (const auto& c : g_groundCubes)
	{
		c.Draw();

		if (DebugDraw_Allow(DebugDrawCategory::Collision))
		{
			Collision_DebugDraw(c.GetAABB(), { 1.0f, 0.0f, 0.0f, 1.0f });
		}
	}
	
	for (const auto& c : g_wallCubes)
	{
		c.Draw();

		if (DebugDraw_Allow(DebugDrawCategory::Collision))
		{
			Collision_DebugDraw(c.GetAABB(), { 1.0f, 0.0f, 0.0f, 1.0f });
		}
	}

	PrimitiveTool::Draw();
}

void Demo_DrawShadow()
{
	for (const auto& c : g_groundCubes)
	{
		c.DrawDepth();
	}

	for (const auto& c : g_wallCubes)
	{
		c.DrawDepth();
	}

	PrimitiveTool::DrawDepth();
}

void Demo_UpdateWorldAABB()
{
	for (auto& c : g_groundCubes)
	{
		c.UpdateAABB();
	}

	for (auto& c : g_wallCubes)
	{
		c.UpdateAABB();
	}

	PrimitiveTool::UpdateAABB();
}

void Demo_AddColliders()
{
	for (const auto& c : g_groundCubes)
	{
		CollisionSystem::AddCollidersAABB(c.GetAABB());
	}

	for (const auto& c : g_wallCubes)
	{
		CollisionSystem::AddCollidersAABB(c.GetAABB());
	}
}

void Demo_RebuildColliders()
{
	CollisionSystem::ClearColliders();
	Demo_AddColliders();
	PrimitiveTool::AppendColliders();
}

static std::array<CubeObject, X_GROUND_CUBE_COUNT * Z_GROUND_CUBE_COUNT> GenerateGroundCubePos(float halfExtent, float y)
{
	std::array<CubeObject, X_GROUND_CUBE_COUNT * Z_GROUND_CUBE_COUNT> cubes{};

	const float step  = halfExtent * 2.0f;
	const int halfX = X_GROUND_CUBE_COUNT / 2;
	const int halfZ = Z_GROUND_CUBE_COUNT / 2;

	int idx = 0;
	for (int z = -halfZ; z <= halfZ; z++)
	{
		for (int x = -halfX; x <= halfX; x++)
		{
			cubes[idx] = CubeObject(halfExtent);
			cubes[idx].SetPosition({ x * step, y, z * step });
			idx++;
		}
	}

	return cubes;
}

static std::array<CubeObject, WALL_CUBE_COUNT> GenerateWallCubePos(float halfExtent, float startY)
{
	std::array<CubeObject, WALL_CUBE_COUNT> cubes{};

	const float step = halfExtent * 2.0f;
	const int halfX = X_GROUND_CUBE_COUNT / 2;
	const int halfZ = Z_GROUND_CUBE_COUNT / 2;

	int idx = 0;
	for (int y = 0; y < Y_WALL_CUBE_HEIGHT; y++)
	{
		for (int z = -halfZ; z <= halfZ + 1; z++)
		{
			cubes[idx] = CubeObject(halfExtent);
			cubes[idx].SetPosition({ (-halfX - 1) * step, startY + y * step, z * step});
			idx++;
		}
	}

	idx = Y_WALL_CUBE_HEIGHT * (X_GROUND_CUBE_COUNT + 1);
	for (int y = 0; y < Y_WALL_CUBE_HEIGHT; y++)
	{
		for (int x = -halfX; x <= halfX + 1; x++)
		{
			cubes[idx] = CubeObject(halfExtent);
			cubes[idx].SetPosition({ x * step, startY + y * step, (halfZ + 1) * step });
			idx++;
		}
	}

	idx = (Y_WALL_CUBE_HEIGHT * (X_GROUND_CUBE_COUNT + 1)) * 2;
	for (int y = 0; y < Y_WALL_CUBE_HEIGHT; y++)
	{
		for (int z = halfZ; z >= -halfZ - 1; z--)
		{
			cubes[idx] = CubeObject(halfExtent);
			cubes[idx].SetPosition({ (halfX + 1) * step, startY + y * step, z * step });
			idx++;
		}
	}

	idx = (Y_WALL_CUBE_HEIGHT * (X_GROUND_CUBE_COUNT + 1)) * 3;
	for (int y = 0; y < Y_WALL_CUBE_HEIGHT; y++)
	{
		for (int x = halfX; x >= -halfX - 1; x--)
		{
			cubes[idx] = CubeObject(halfExtent);
			cubes[idx].SetPosition({ x * step, startY + y * step, (-halfZ - 1) * step });
			idx++;
		}
	}
	
	return cubes;
}
