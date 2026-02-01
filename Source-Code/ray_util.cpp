/*==============================================================================

   Picking from camera to screen point (ray casting) [ray_util.cpp]
														 Author : Gu Anyi
														 Date   : 2026/02/01
--------------------------------------------------------------------------------

==============================================================================*/

#include "ray_util.h"
#include "direct3d.h"
#include "camera_base.h"

using namespace DirectX;

void BuildRayFromScreen(
	const CameraBase& cam,
	int mouseX,
	int mouseY,
	XMVECTOR& outRayOrigin,
	XMVECTOR& outRayDir)
{
	// screen to NDC
	float sx = (2.0f * mouseX) / Direct3D_GetBackBufferWidth() - 1.0f;
	float sy = 1.0f - (2.0f * mouseY) / Direct3D_GetBackBufferHeight();

	// NDC to clip space
	XMVECTOR rayClip = XMVectorSet(sx, sy, 1.0f, 1.0f);

	// clip to view
	XMMATRIX invProj = XMLoadFloat4x4(&cam.GetInvProj());

	XMVECTOR rayView = XMVector4Transform(rayClip, invProj);
	rayView = XMVectorSetZ(rayView, 1.0f);
	rayView = XMVectorSetW(rayView, 0.0f);

	// view to world
	XMMATRIX invView = XMLoadFloat4x4(&cam.GetInvView());

	XMVECTOR rayWorld = XMVector3Normalize(XMVector3TransformNormal(rayView, invView));

	outRayOrigin = XMLoadFloat3(&cam.GetPosition());
	outRayDir = rayWorld;
}
