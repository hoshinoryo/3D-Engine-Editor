/*==============================================================================

   Translate gizmo API [gizmo_translate.cpp]
														 Author : Gu Anyi
														 Date   : 2026/02/01
--------------------------------------------------------------------------------

==============================================================================*/

#include "gizmo_translate.h"
#include "scene_manager.h"
#include "mesh_object.h"
#include "draw3d.h"
#include "camera_manager.h"
#include "mouse.h"
#include "ray_util.h"
#include "direct3d.h"
#include "camera_base.h"
#include "collision.h"
#include "editor_tool_draw_gate.h"
#include "key_logger.h"

using namespace DirectX;

struct Vec2 { float x, y; };

namespace
{
	GizmoAxis g_ActiveAxis = GizmoAxis::None;
	bool g_Dragging = false;

	XMMATRIX g_View;
	XMMATRIX g_Proj;

	XMFLOAT3 g_StartWorldPos{};
	XMFLOAT3 g_StartGizmoPos{};
	XMFLOAT3 g_AxisDir{};
	float g_StartS = 0.0f;

	XMFLOAT3 g_PivotToTransformOffset{};
	XMFLOAT3 g_DraggingPivot{};

	XMFLOAT3* g_pExternalPos = nullptr;
	bool g_ExternalDirty = false;

	ID3D11DepthStencilState* g_pGizmoDepthOff = nullptr; // gizmo always on top layer
}

static XMFLOAT4 AxisColor(GizmoAxis axis);
static void DrawGizmo(const XMFLOAT3& pivot);

static void EnsureGizmoDepthState();
static void SetGizmoDepthOff(ID3D11Device* ctx, ID3D11DepthStencilState* outPrev, UINT& outPrevRef);
static void RestoreDepthState(ID3D11Device* ctx, ID3D11DepthStencilState* prev, UINT prevRef);

// ---------------
//  Picking Block
// ---------------
static Vec2 WorldToScreen(FXMVECTOR pWorld, CXMMATRIX view, CXMMATRIX proj);
static float DisPointToSegment2D(Vec2 p, Vec2 a, Vec2 b);
// ---------------
//  Mapping Block
// ---------------
static bool RayIntersectPlane(
	FXMVECTOR rayOrigin, FXMVECTOR rayDir,
	FXMVECTOR planeP0, FXMVECTOR planeN,
	float& outT
);
static XMVECTOR MakeDragPlaneNormal(FXMVECTOR axisDir, FXMVECTOR viewDir);
// ----------------------
//  Calculate Drag Block
// ----------------------
static bool BeginDrag(const XMFLOAT3& gizmoPivotWorld, const XMFLOAT3& targetWorldPos, int mouseX, int mouseY);
static bool OnDrag(int mouseX, int mouseY, XMFLOAT3& inOutTargetPos);


static float CalculateAxisProject(int mouseX, int mouseY, FXMVECTOR startPos, FXMVECTOR axisDir);
static XMFLOAT3 GetTranslationFromMatrix(XMMATRIX m);
static XMFLOAT3 GetGizmoPivotWorld(const MeshObject& obj);


void GizmoTranslate::Begin(const XMMATRIX& view, const XMMATRIX& proj)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate))
	{
		return;
	}

	g_View = view;
	g_Proj = proj;
}

void GizmoTranslate::Draw(const MeshObject& obj)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate))
	{
		return;
	}

	if (!obj.asset) return;

	XMFLOAT3 p;
	if (g_Dragging)
	{
		p = g_DraggingPivot;
	}
	else
	{
		p = GetGizmoPivotWorld(obj);
	}

	DrawGizmo(p);
}

// picking
bool GizmoTranslate::OnMouseDown(int mouseX, int mouseY)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate)) return false;

	if (KeyLogger_IsPressed(KK_LEFTALT) || KeyLogger_IsPressed(KK_RIGHTALT)) return false;

	MeshObject* obj = SceneManager::GetSelectedObject();
	if (!obj) return false;

	XMFLOAT3 gizmoPivot = GetGizmoPivotWorld(*obj);

	return BeginDrag(gizmoPivot, obj->transform.position, mouseX, mouseY);
}

// calculate movement
void GizmoTranslate::OnMouseDrag(int mouseX, int mouseY)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate)) return;

	MeshObject* obj = SceneManager::GetSelectedObject();
	if (!obj) return;

	if (OnDrag(mouseX, mouseY, obj->transform.position))
	{
		obj->aabbValid = false;
	}
}

// stop picking
void GizmoTranslate::OnMouseUp()
{
	g_Dragging = false;
	g_ActiveAxis = GizmoAxis::None;
}

bool GizmoTranslate::IsActive()
{
	return g_Dragging;
}

bool GizmoTranslate::OnMouseDownExternal(DirectX::XMFLOAT3& inOutPos, int mouseX, int mouseY)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate)) return false;

	if (KeyLogger_IsPressed(KK_LEFTALT) || KeyLogger_IsPressed(KK_RIGHTALT)) return false;

	g_pExternalPos = &inOutPos;
	g_ExternalDirty = false;

	return BeginDrag(inOutPos, inOutPos, mouseX, mouseY);
}

bool GizmoTranslate::OnMouseDragExternal(DirectX::XMFLOAT3& inOutPos, int mouseX, int mouseY)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate)) return false;

	g_pExternalPos = &inOutPos;

	if (OnDrag(mouseX, mouseY, inOutPos))
	{
		g_ExternalDirty = true;
		return true;
	}

	return false;
}

void GizmoTranslate::DrawExternal(const DirectX::XMFLOAT3& pos)
{
	if (!EditorTool_Allow(EditorToolCategory::GizmoTranslate)) return;

	XMFLOAT3 pivot = (g_Dragging) ? g_DraggingPivot : pos;

	DrawGizmo(pivot);
}

static XMFLOAT4 AxisColor(GizmoAxis axis)
{
	switch (axis)
	{
	case GizmoAxis::X: return { 1.0f, 0.0f, 0.0f, 1.0f };
	case GizmoAxis::Y: return { 0.0f, 1.0f, 0.0f, 1.0f };
	case GizmoAxis::Z: return { 0.0f, 0.0f, 1.0f, 1.0f };
	default:           return { 1.0f, 1.0f, 1.0f, 1.0f };
	}
}

static void DrawGizmo(const XMFLOAT3& pivot)
{
	const float len = 2.5f;

	Draw3d_MakeThickLine(pivot, { pivot.x + len, pivot.y,       pivot.z }, 0.1f, AxisColor(GizmoAxis::X));
	Draw3d_MakeThickLine(pivot, { pivot.x,       pivot.y + len, pivot.z }, 0.1f, AxisColor(GizmoAxis::Y));
	Draw3d_MakeThickLine(pivot, { pivot.x,       pivot.y,       pivot.z + len }, 0.1f, AxisColor(GizmoAxis::Z));
}

static void EnsureGizmoDepthState()
{
	if (g_pGizmoDepthOff) return;

	ID3D11Device* dev = Direct3D_GetDevice();
	if (!dev) return;

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = FALSE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ds.StencilEnable = FALSE;

	dev->CreateDepthStencilState(&ds, &g_pGizmoDepthOff);
}

void SetGizmoDepthOff(ID3D11Device* ctx, ID3D11DepthStencilState* outPrev, UINT& outPrevRef)
{
	outPrev = nullptr;
	outPrevRef = 0;

	EnsureGizmoDepthState();
	if (!ctx || !g_pGizmoDepthOff) return;


}

void RestoreDepthState(ID3D11Device* ctx, ID3D11DepthStencilState* prev, UINT prevRef)
{
}

static Vec2 WorldToScreen(FXMVECTOR pWorld, CXMMATRIX view, CXMMATRIX proj)
{
	const float w = (float)Direct3D_GetBackBufferWidth();
	const float h = (float)Direct3D_GetBackBufferHeight();

	XMMATRIX vp = XMMatrixMultiply(view, proj);
	XMVECTOR clip = XMVector4Transform(XMVectorSetW(pWorld, 1.0f), vp);

	float cw = XMVectorGetW(clip);
	if (fabs(cw) < 1e-6f) return { -FLT_MAX, -FLT_MAX };

	XMVECTOR ndc = clip / cw;
	float nx = XMVectorGetX(ndc);
	float ny = XMVectorGetY(ndc);

	// NDC to screen
	float sx = (nx * 0.5f + 0.5f) * w;
	float sy = (1.0f - (ny * 0.5f + 0.5f)) * h;

	return { sx, sy };
}

static float DisPointToSegment2D(Vec2 p, Vec2 a, Vec2 b)
{
	float vx = b.x - a.x;
	float vy = b.y - a.y;
	float wx = p.x - a.x;
	float wy = p.y - a.y;

	float vv = vx * vx + vy * vy;
	if (vv < 1e-6f)
	{
		float dx = p.x - a.x;
		float dy = p.y - a.y;
		return sqrtf(dx * dx + dy * dy);
	}

	float t = (wx * vx + wy * vy) / vv;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	float cx = a.x + t * vx;
	float cy = a.y + t * vy;

	float dx = p.x - cx;
	float dy = p.y - cy;
	return sqrtf(dx * dx + dy * dy);
}

static bool RayIntersectPlane(FXMVECTOR rayOrigin, FXMVECTOR rayDir, FXMVECTOR planeP0, FXMVECTOR planeN, float& outT)
{
	float denom = XMVectorGetX(XMVector3Dot(rayDir, planeN));
	if (fabs(denom) < 1e-6f) return false;

	float numer = XMVectorGetX(XMVector3Dot(planeP0 - rayOrigin, planeN));
	outT = numer / denom;
	return outT >= 0.0f;
}

XMVECTOR MakeDragPlaneNormal(FXMVECTOR axisDir, FXMVECTOR viewDir)
{
	XMVECTOR vxa = XMVector3Cross(viewDir, axisDir); // side vector

	float len = XMVectorGetX(XMVector3LengthSq(vxa));
	if (len < 1e-6f)
	{
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		vxa = XMVector3Cross(up, axisDir);
	}

	XMVECTOR n = XMVector3Cross(axisDir, vxa); // normal vector

	return XMVector3Normalize(n);
}

static bool BeginDrag(const XMFLOAT3& gizmoPivotWorld, const XMFLOAT3& targetWorldPos, int mouseX, int mouseY)
{
	g_StartWorldPos = targetWorldPos;
	g_StartGizmoPos = gizmoPivotWorld;
	g_DraggingPivot = gizmoPivotWorld;

	g_PivotToTransformOffset = {
		g_StartWorldPos.x - gizmoPivotWorld.x,
		g_StartWorldPos.y - gizmoPivotWorld.y,
		g_StartWorldPos.z - gizmoPivotWorld.z
	};

	const float len = 2.5f;

	// screen screen picking
	XMVECTOR o = XMLoadFloat3(&gizmoPivotWorld);
	XMVECTOR x1 = o + XMVectorSet(len, 0, 0, 0);
	XMVECTOR y1 = o + XMVectorSet(0, len, 0, 0);
	XMVECTOR z1 = o + XMVectorSet(0, 0, len, 0);

	Vec2 so = WorldToScreen(o, g_View, g_Proj);
	Vec2 sx = WorldToScreen(x1, g_View, g_Proj);
	Vec2 sy = WorldToScreen(y1, g_View, g_Proj);
	Vec2 sz = WorldToScreen(z1, g_View, g_Proj);

	Vec2 m{ (float)mouseX, (float)mouseY };

	float dx = DisPointToSegment2D(m, so, sx);
	float dy = DisPointToSegment2D(m, so, sy);
	float dz = DisPointToSegment2D(m, so, sz);

	const float kPickPx = 12.0f;
	g_ActiveAxis = GizmoAxis::None;

	float best = kPickPx;
	if (dx < best)
	{
		best = dx;
		g_ActiveAxis = GizmoAxis::X;
		g_AxisDir = { 1, 0, 0 };
	}
	if (dy < best)
	{
		best = dy;
		g_ActiveAxis = GizmoAxis::Y;
		g_AxisDir = { 0, 1, 0 };
	}
	if (dz < best)
	{
		best = dz;
		g_ActiveAxis = GizmoAxis::Z;
		g_AxisDir = { 0, 0, 1 };
	}

	if (g_ActiveAxis == GizmoAxis::None) return false;

	g_Dragging = true;

	XMVECTOR axisDirV = XMVector3Normalize(XMLoadFloat3(&g_AxisDir));
	XMVECTOR gizmoPosV = XMLoadFloat3(&g_StartGizmoPos);

	g_StartS = CalculateAxisProject(mouseX, mouseY, gizmoPosV, axisDirV);
	return true;
}

static bool OnDrag(int mouseX, int mouseY, XMFLOAT3& inOutTargetPos)
{
	if (!g_Dragging) return false;

	XMVECTOR axisDirV = XMVector3Normalize(XMLoadFloat3(&g_AxisDir));
	XMVECTOR startPivotV = XMLoadFloat3(&g_StartGizmoPos);

	float sNow = CalculateAxisProject(mouseX, mouseY, startPivotV, axisDirV);
	float delta = sNow - g_StartS;

	XMVECTOR newPivotV = startPivotV + axisDirV * delta;

	XMFLOAT3 newPivot;
	XMStoreFloat3(&newPivot, newPivotV);
	g_DraggingPivot = newPivot;

	inOutTargetPos = {
		newPivot.x + g_PivotToTransformOffset.x,
		newPivot.y + g_PivotToTransformOffset.y,
		newPivot.z + g_PivotToTransformOffset.z,
	};

	return true;
}

static float CalculateAxisProject(int mouseX, int mouseY, FXMVECTOR startPos, FXMVECTOR axisDir)
{
	CameraBase& cam = CameraManager::GetActiveCamera();

	XMVECTOR rayO, rayD;
	BuildRayFromScreen(cam, mouseX, mouseY, rayO, rayD);
	
	XMFLOAT3 f = cam.GetFront();
	XMVECTOR viewDir = XMVector3Normalize(XMLoadFloat3(&f));

	// build plane normal
	XMVECTOR planeN = MakeDragPlaneNormal(axisDir, viewDir);
	float t;
	if (RayIntersectPlane(rayO, rayD, startPos, planeN, t))
	{
		XMVECTOR hit = rayO + rayD * t;
		return XMVectorGetX(XMVector3Dot(hit - startPos, axisDir));
	}
	
	return 0.0f;
}

static XMFLOAT3 GetTranslationFromMatrix(XMMATRIX m)
{
	XMFLOAT3 t{};
	XMStoreFloat3(&t, m.r[3]);
	return t;
}

XMFLOAT3 GetGizmoPivotWorld(const MeshObject& obj)
{
	// world AABB center = what you actully see
	if (obj.aabbValid)
	{
		return obj.worldAABB.GetCenter();
	}

	XMMATRIX instanceWorld = obj.transform.ToMatrix();
	XMMATRIX nodeToModel = XMMatrixIdentity();

	if (obj.asset && obj.meshIndex < obj.asset->meshes.size())
	{
		nodeToModel = XMLoadFloat4x4(&obj.asset->meshes[obj.meshIndex].nodeToModel);
	}

	XMMATRIX world = nodeToModel * instanceWorld;
	return GetTranslationFromMatrix(world);
}
