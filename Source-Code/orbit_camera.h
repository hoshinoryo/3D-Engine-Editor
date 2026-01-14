/*==============================================================================

   Orbit camera control [orbit_camera.h]
														 Author : Gu Anyi
														 Date   : 2025/10/29
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include "debug_text.h"
#include "camera_base.h"
#include "mouse.h"

#include <DirectXMath.h>

class OrbitCamera : public CameraBase
{
private:

	DirectX::XMFLOAT3 m_CameraPosition;
	DirectX::XMFLOAT3 m_CameraUp;

	float m_CameraFov;

	// Orbit mode attribute
	DirectX::XMFLOAT3 m_CameraTarget;
	float             m_CameraRadius;
	float             m_CameraYaw;   // Radian
	float             m_CameraPitch; // Radian

	DirectX::XMFLOAT4X4 m_View;
	DirectX::XMFLOAT4X4 m_Proj;

	ID3D11Buffer* m_pVSConstantBufferView = nullptr; // matrix for world to view(b1)
	ID3D11Buffer* m_pVSConstantBufferProj = nullptr; // matrix for view to clip(b2)

	int m_LastMouseX;
	int m_LastMouseY;

	bool m_KeyRotationEnabled = true;

	void HandleKeyInput(double elapsed_time, bool enableRotation);
	void HandleMouseInput(double elapsed_time, const Mouse_State& mouseState);

public:

	OrbitCamera();
	~OrbitCamera();

	bool Initialize();
	void Finalize();

	void Update(double elasped_time) override;

	const DirectX::XMFLOAT4X4& GetView() const override { return m_View; }
	const DirectX::XMFLOAT4X4& GetProj() const override { return m_Proj; }
	const DirectX::XMFLOAT3& GetPosition() const override { return m_CameraPosition; };
	DirectX::XMFLOAT3 GetFront() const override;

	float GetFov();

	void DebugDraw();

	void SetKeyRotationEnabled(bool enabled) {
		m_KeyRotationEnabled = enabled;
	}
};

#endif // ORBIT_CAMERA_H
