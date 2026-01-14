/*==============================================================================

   Render management [render3d.cpp]
   Combine light and shader
														 Author : Gu Anyi
														 Date   : 2025/11/18
--------------------------------------------------------------------------------

==============================================================================*/

#include "render3d.h"
//#include "orbit_camera.h"
//#include "player_camera.h"
#include "camera_base.h"
#include "light.h"

void Render3D_BeginFrame(const CameraBase& camera)
{
	// Update light manager
	g_LightManager.BindAllLightsToPipeline();
}
