/*==============================================================================

   Player camera control [player_camera.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include "player_camera.h"
#include "direct3d.h"
//#include "key_logger.h"
#include "mouse.h"

#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;


static const float MIN_PITCH = XMConvertToRadians(91.0f);
static const float MAX_PITCH = XMConvertToRadians(179.0f);


bool PlayerCamera::Initialize()
{
	ID3D11Device* device = Direct3D_GetDevice();
	if (!device) return false;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;

	if (FAILED(device->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferView)))
		return false;

	if (FAILED(device->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferProj)))
		return false;

	XMStoreFloat4x4(&m_View, XMMatrixIdentity());
	XMStoreFloat4x4(&m_Proj, XMMatrixIdentity());

	return true;
}

void PlayerCamera::Finalize()
{
	if (m_pVSConstantBufferView)
	{
		m_pVSConstantBufferView->Release();
		m_pVSConstantBufferView = nullptr;
	}

	if (m_pVSConstantBufferProj)
	{
		m_pVSConstantBufferProj->Release();
		m_pVSConstantBufferProj = nullptr;
	}
}

void PlayerCamera::SetFollowTarget(const XMFLOAT3* playerPos)
{
	m_pPlayerPos = playerPos;
}

void PlayerCamera::Update(double elapsed_time)
{
	if (!m_Active || !m_pPlayerPos) return;

	UpdateInput(elapsed_time);
	UpdateMatrices();

	ID3D11DeviceContext* ctx = Direct3D_GetContext();
	ctx->VSSetConstantBuffers(1, 1, &m_pVSConstantBufferView);
	ctx->VSSetConstantBuffers(2, 1, &m_pVSConstantBufferProj);
}

void PlayerCamera::UpdateInput(double elapsed_time)
{
	Mouse_State mouseState{};
	Mouse_GetState(&mouseState);

	float mouse_dx = (float)(mouseState.x - m_LastMouseX);
	float mouse_dy = (float)(mouseState.y - m_LastMouseY);
	m_LastMouseX = mouseState.x;
	m_LastMouseY = mouseState.y;

	bool enableLook = true;

	if (enableLook)
	{
		m_Yaw   += m_MouseSensitivity * mouse_dx;
		m_Yaw = fmodf(m_Yaw, XM_2PI);
		if (m_Yaw < 0) m_Yaw += XM_2PI;

		m_Pitch += m_MouseSensitivity * mouse_dy;
		m_Pitch = std::max(MIN_PITCH, std::min(MAX_PITCH, m_Pitch));
	}

	if (mouseState.scrollWheelValue != 0)
	{
		m_Distance -= float(mouseState.scrollWheelValue) * 0.01f;
		Mouse_ResetScrollWheelValue();
	}
	m_Distance = std::max(1.5f, std::min(20.0f, m_Distance));
}

void PlayerCamera::UpdateMatrices()
{
	XMVECTOR player = XMLoadFloat3(m_pPlayerPos);

	float sinPitch = sinf(m_Pitch);
	float cosPitch = cosf(m_Pitch);
	float sinYaw = sinf(m_Yaw);
	float cosYaw = cosf(m_Yaw);

	// Camera forward
	XMVECTOR forward = XMVector3Normalize(XMVectorSet(sinPitch * sinYaw, cosPitch, sinPitch * cosYaw, 0.0f));
	XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);

	XMVECTOR camPos = player - forward * m_Distance + worldUp * m_Height;
	XMVECTOR target = player + worldUp * m_LookHeight;

	XMStoreFloat3(&m_Position, camPos);
	XMVECTOR camFront = XMVector3Normalize(target - camPos);
	XMStoreFloat3(&m_Front, camFront);

	XMMATRIX view = XMMatrixLookAtLH(camPos, target, worldUp);
	XMStoreFloat4x4(&m_View, view);

	// Perspective array
	float fov = XMConvertToRadians(60.0f);
	float aspectRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	float nearZ = 0.1f;
	float farZ = 100.0f;
	XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
	XMStoreFloat4x4(&m_Proj, proj);

	XMFLOAT4X4 viewT, projT;
	XMStoreFloat4x4(&viewT, XMMatrixTranspose(view));
	XMStoreFloat4x4(&projT, XMMatrixTranspose(proj));

	ID3D11DeviceContext* ctx = Direct3D_GetContext();
	ctx->UpdateSubresource(m_pVSConstantBufferView, 0, nullptr, &viewT, 0, 0);
	ctx->UpdateSubresource(m_pVSConstantBufferProj, 0, nullptr, &projT, 0, 0);
}
