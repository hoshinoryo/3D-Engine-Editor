/*==============================================================================

   Player camera control [player_camera.h]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include <DirectXMath.h>
#include <d3d11.h>

#include "camera_base.h"

class PlayerCamera : public CameraBase
{
public:

	bool Initialize();
	void Finalize();

	void SetFollowTarget(const DirectX::XMFLOAT3* playerPos);

	void SetActive(bool v) { m_Active = v; }
	bool IsActive() const { return m_Active; }

	void Update(double elapsed_time) override;

	const DirectX::XMFLOAT4X4& GetView() const override { return m_View; }
	const DirectX::XMFLOAT4X4& GetProj() const override { return m_Proj; }
	const DirectX::XMFLOAT3& GetPosition() const override { return m_Position; }
	DirectX::XMFLOAT3 GetFront() const override { return m_Front; }

	float GetYaw() const { return m_Yaw; }

private:

	void UpdateInput(double elapsed_time);
	void UpdateMatrices();

private:

	bool m_Active = false;

	const DirectX::XMFLOAT3* m_pPlayerPos = nullptr;

	float m_Yaw   = DirectX::XMConvertToRadians(180.0f);
	float m_Pitch = DirectX::XMConvertToRadians(20.0f);

	float m_Distance = 6.0f;
	float m_Height = 5.0f;
	float m_LookHeight = 1.5f;

	float m_MouseSensitivity = 0.003f;

	DirectX::XMFLOAT4X4 m_View{};
	DirectX::XMFLOAT4X4 m_Proj{};
	DirectX::XMFLOAT3 m_Position{};
	DirectX::XMFLOAT3 m_Front{};

	ID3D11Buffer* m_pVSConstantBufferView = nullptr;
	ID3D11Buffer* m_pVSConstantBufferProj = nullptr;

	int m_LastMouseX = 0;
	int m_LastMouseY = 0;

};


