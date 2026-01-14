/*==============================================================================

   Camera management and mode switch [camera_manager.h]
														 Author : Gu Anyi
														 Date   : 2026/01/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <DirectXMath.h>

class CameraBase;
class OrbitCamera;
class PlayerCamera;
class Player;

extern bool g_IsPlayMode;

namespace CameraManager
{
	bool Initialize(Player* player);
	void Finalize();

	void Update(double elapsed_time);

	void SetPlayerMode(bool play);
	bool IsPlayMode();

	// Guide drawing judgment
	void NotifyEnterPlayMode();
	void NotifyEnterEditorMode();

	// Get the camera
	CameraBase& GetActiveCamera();
	OrbitCamera& GetOrbitCamera();
	PlayerCamera& GetPlayerCamera();
}

#endif // CAMERA_MANAGER_H
