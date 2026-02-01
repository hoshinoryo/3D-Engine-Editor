/*==============================================================================

   Picking from camera to screen point (ray casting) [ray_util.h]
														 Author : Gu Anyi
														 Date   : 2026/02/01
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef RAY_UTIL_H
#define RAY_UTIL_H

#include <DirectXMath.h>

class CameraBase;

void BuildRayFromScreen(
	const CameraBase& cam,
	int mouseX,
	int mouseY,
	DirectX::XMVECTOR& outRayOrigin,
	DirectX::XMVECTOR& outRayDir
);

#endif // RAY_UTIL_H
