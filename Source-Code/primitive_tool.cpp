/*==============================================================================

　 プリミティブ生成管理ツール[primitive_tool.cpp]
														 Author : Gu Anyi
														 Date   : 2026/02/10
--------------------------------------------------------------------------------

==============================================================================*/

#include <vector>
#include <algorithm>

#include "primitive_tool.h"
#include "ray_util.h"
#include "gizmo_translate.h"
#include "collision.h"
#include "imgui/imgui.h"
#include "debug_draw_gate.h"
#include "picking_pass.h"

using namespace DirectX;

static bool RayIntersectsAABB(FXMVECTOR rayOrigin, FXMVECTOR rayDir, const AABB& aabb, float& outT);
static bool IntersectSlab(float o, float d, float minv, float maxv, float& tmin, float& tmax);

namespace
{
	struct PrimCube
	{
		CubeObject cube;
		uint32_t id = 0;
	};

	std::vector<PrimCube> g_Cubes;
	int g_Selected = -1;
	float g_HalfExtent = 1.0f;

	uint32_t g_NextPrimId = 1;
	static uint32_t AllocPrimId()
	{
		return 0x80000000u | (g_NextPrimId++);
	}
}

void PrimitiveTool::Initialize(float cubeHalfExtent)
{
	g_HalfExtent = cubeHalfExtent;
	g_Cubes.clear();
	g_Selected = -1;
	g_NextPrimId = 1;
}

void PrimitiveTool::Finalize()
{
	g_Cubes.clear();
	g_Selected = -1;
}

void PrimitiveTool::CreateCube(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& scl)
{
	PrimCube p{ CubeObject(g_HalfExtent), AllocPrimId() };
	p.cube.SetPosition(pos);
	p.cube.SetScale(scl);
	p.cube.UpdateAABB();

	g_Cubes.push_back(p);
	g_Selected = (int)g_Cubes.size() - 1;
}

void PrimitiveTool::DeleteSelected()
{
	if (g_Selected < 0 || g_Selected >= (int)g_Cubes.size()) return;

	g_Cubes.erase(g_Cubes.begin() + g_Selected);

	if (g_Cubes.empty())
	{
		g_Selected = -1;
		return;
	}

	g_Selected = std::min(g_Selected, (int)g_Cubes.size() - 1);
}

void PrimitiveTool::DuplicateSelected()
{
	if (!HasSelection()) return;

	PrimCube copy = g_Cubes[g_Selected];
	copy.id = AllocPrimId();
	copy.cube.UpdateAABB();

	g_Cubes.push_back(copy);
	g_Selected = (int)g_Cubes.size() - 1;
}

void PrimitiveTool::ClearSelection()
{
	g_Selected = -1;
}

bool PrimitiveTool::HasSelection()
{
	return (g_Selected >= 0 && g_Selected < (int)g_Cubes.size());
}

CubeObject* PrimitiveTool::GetSelected()
{
	if (!HasSelection()) return nullptr;
	return &g_Cubes[g_Selected].cube;
}

int PrimitiveTool::GetSelectedIndex()
{
	return g_Selected;
}

uint32_t PrimitiveTool::GetSelectedObjectId()
{
	if (!HasSelection()) return 0;
	return g_Cubes[g_Selected].id;
}

int PrimitiveTool::GetCount()
{
	return (int)g_Cubes.size();
}

bool PrimitiveTool::PickFromMouse(const CameraBase& cam, int mouseX, int mouseY)
{
	XMVECTOR rayOrigin{};
	XMVECTOR rayDir{};
	BuildRayFromScreen(cam, mouseX, mouseY, rayOrigin, rayDir);

	float bestT = 1e30f;
	int bestIdx = -1;

	for (int i = 0; i < (int)g_Cubes.size(); i++)
	{
		float t = 0.0f;
		if (RayIntersectsAABB(rayOrigin, rayDir, g_Cubes[i].cube.GetAABB(), t))
		{
			if (t > 0.0f && t < bestT)
			{
				bestT = t;
				bestIdx = i;
			}
		}
	}

	if (bestIdx >= 0)
	{
		g_Selected = bestIdx;
		return true;
	}
	return false;
}

void PrimitiveTool::Draw()
{
	for (auto& p : g_Cubes)
	{
		p.cube.Draw();

		if (DebugDraw_Allow(DebugDrawCategory::Collision))
		{
			Collision_DebugDraw(p.cube.GetAABB(), { 1.0f, 0.0f, 0.0f, 1.0f });
		}
	}
}

void PrimitiveTool::UpdateAABB()
{
	for (auto& p : g_Cubes)
	{
		p.cube.UpdateAABB();
	}
}

void PrimitiveTool::AppendColliders()
{
	for (const auto& p : g_Cubes)
	{
		CollisionSystem::AddCollidersAABB(p.cube.GetAABB());
	}
}

void PrimitiveTool::MenuDraw()
{
	if (ImGui::Button("Create Cube", ImVec2(-1, 0)))
	{
		CreateCube({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	}

	if (ImGui::Button("Duplicate Cube", ImVec2(-1, 0)))
	{
		DuplicateSelected();
	}

	if (ImGui::Button("Delete Selected", ImVec2(-1, 0)))
	{
		DeleteSelected();
	}

	ImGui::Separator();
	ImGui::Text("Count: %d", (int)g_Cubes.size());
	ImGui::Text("Selected: %d", g_Selected);
}

void PrimitiveTool::DrawPicking(PickingPass& pass)
{
	ID3D11Buffer* vb = Cube_GetVB();
	ID3D11Buffer* ib = Cube_GetIB();
	UINT indexCount  = Cube_GetIndexCount();
	UINT stride      = Cube_GetVBStride();
	UINT offset      = 0;

	if (!vb || !ib || indexCount == 0) return;

	for (const auto& p : g_Cubes)
	{
		const XMFLOAT3 pos = p.cube.GetPosition();
		const XMFLOAT3 scl = p.cube.GetScale();
		
		XMMATRIX world = XMMatrixScaling(scl.x, scl.y, scl.z) * XMMatrixTranslation(pos.x, pos.y, pos.z);

		pass.DrawIndexed(vb, stride, offset, ib, Cube_GetIBFormat(), 0, indexCount, world, p.id);
	}
}

static bool RayIntersectsAABB(FXMVECTOR rayOrigin, FXMVECTOR rayDir, const AABB& aabb, float& outT)
{
	XMFLOAT3 origin{}, dir{};
	XMStoreFloat3(&origin, rayOrigin);
	XMStoreFloat3(&dir, rayDir);

	float tmin = 0.0f;
	float tmax = FLT_MAX;

	if (!IntersectSlab(origin.x, dir.x, aabb.min.x, aabb.max.x, tmin, tmax)) return false;
	if (!IntersectSlab(origin.y, dir.y, aabb.min.y, aabb.max.y, tmin, tmax)) return false;
	if (!IntersectSlab(origin.z, dir.z, aabb.min.z, aabb.max.z, tmin, tmax)) return false;

	outT = tmin;
	return true;
}

// Ray-AABB slab method
static bool IntersectSlab(float o, float d, float minv, float maxv, float& tmin, float& tmax)
{
	const float EPS = 1e-6f;

	if (fabsf(d) < EPS)
	{
		return (o >= minv && o <= maxv);
	}

	float inv = 1.0f / d;
	float t1 = (minv - o) * inv;
	float t2 = (maxv - o) * inv;

	if (t1 > t2) std::swap(t1, t2);

	tmin = (t1 > tmin) ? t1 : tmin;
	tmax = (t2 < tmax) ? t2 : tmax;

	return (tmin <= tmax);
}
