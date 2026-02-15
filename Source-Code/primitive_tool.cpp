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

using namespace DirectX;

static bool RayIntersectsAABB(FXMVECTOR rayOrigin, FXMVECTOR rayDir, const AABB& aabb, float& outT);
static bool IntersectSlab(float o, float d, float minv, float maxv, float& tmin, float& tmax);

namespace
{
	std::vector<CubeObject> g_Cubes;
	int g_Selected = -1;
	float g_HalfExtent = 1.0f;
}

void PrimitiveTool::Initialize(float cubeHalfExtent)
{
	g_HalfExtent = cubeHalfExtent;
	g_Cubes.clear();
	g_Selected = -1;
}

void PrimitiveTool::Finalize()
{
	g_Cubes.clear();
	g_Selected = -1;
}

void PrimitiveTool::CreateCube(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& scl)
{
	CubeObject c(g_HalfExtent);
	c.SetPosition(pos);
	c.SetScale(scl);

	g_Cubes.push_back(c);
	g_Selected = (int)g_Cubes.size() - 1;

	g_Cubes[g_Selected].UpdateAABB();
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
	return &g_Cubes[g_Selected];
}

int PrimitiveTool::GetSelectedIndex()
{
	return g_Selected;
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
		if (RayIntersectsAABB(rayOrigin, rayDir, g_Cubes[i].GetAABB(), t))
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
	for (auto& c : g_Cubes)
	{
		c.Draw();

		if (DebugDraw_Allow(DebugDrawCategory::Collision))
		{
			Collision_DebugDraw(c.GetAABB(), { 1.0f, 0.0f, 0.0f, 1.0f });
		}
	}
}

void PrimitiveTool::UpdateAABB()
{
	for (auto& c : g_Cubes)
	{
		c.UpdateAABB();
	}
}

void PrimitiveTool::AppendColliders()
{
	for (const auto& c : g_Cubes)
	{
		CollisionSystem::AddCollidersAABB(c.GetAABB());
	}
}

void PrimitiveTool::MenuDraw()
{
	if (ImGui::Button("Create Cube", ImVec2(-1, 0)))
	{
		CreateCube({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	}

	if (ImGui::Button("Delete Selected", ImVec2(-1, 0)))
	{
		DeleteSelected();
	}

	ImGui::Separator();
	ImGui::Text("Count: %d", (int)g_Cubes.size());
	ImGui::Text("Selected: %d", g_Selected);
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
