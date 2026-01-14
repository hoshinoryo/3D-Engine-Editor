/*==============================================================================

   Render management [render3d.h]
   Combine light and shader (frame-level)
														 Author : Gu Anyi
														 Date   : 2025/11/18
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef RENDER3D_H
#define RENDER3D_H

#include <DirectXMath.h>

class CameraBase;

void Render3D_BeginFrame(const CameraBase& camera);

#endif // RENDER3D_H
