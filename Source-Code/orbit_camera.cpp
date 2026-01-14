/*==============================================================================

   Orbit camera control [orbit_camera.cpp]
														 Author : Gu Anyi
														 Date   : 2025/10/29
--------------------------------------------------------------------------------

==============================================================================*/

#include "orbit_camera.h"
#include "direct3d.h"
#include "key_logger.h"

#include <DirectXMath.h>
#include <sstream>
#include <algorithm>
#include <d3d11.h>

#include "imgui/imgui.h"


using namespace DirectX;


static const float MIN_PITCH = XMConvertToRadians(1.0f);
static const float MAX_PITCH = XMConvertToRadians(179.0f);

static const float CAMERA_ROTATE_SPEED_ORBIT = XMConvertToRadians(90);
static const float CAMERA_PAN_SENSITIVITY = 0.002f;
static const float CAMERA_ROTATE_SENSITIVITY = 0.002f;
static const float CAMERA_ZOOM_SPEED = 0.01f;
static const float CAMERA_PAN_SPEED = 3.0f;

// Control keys
static const Keyboard_Keys KEY_UP = KK_UP;
static const Keyboard_Keys KEY_DOWN = KK_DOWN;
static const Keyboard_Keys KEY_LEFT = KK_LEFT;
static const Keyboard_Keys KEY_RIGHT = KK_RIGHT;


OrbitCamera::OrbitCamera()
{
	m_CameraPosition = { 0.0f, 0.0f, 0.0f };
	m_CameraUp = { 0.0f, 1.0f, 0.0f };

	m_CameraFov = 60.0f;

	m_CameraTarget = { 0.0f, 0.0f, 0.0f };
	m_CameraRadius = 10.0f;
	m_CameraYaw = XMConvertToRadians(180.0f);
	m_CameraPitch = XMConvertToRadians(60.0f);

	// pitch angle is limited
	m_CameraPitch = std::max(MIN_PITCH, std::min(MAX_PITCH, m_CameraPitch));

	XMStoreFloat4x4(&m_View, XMMatrixIdentity());
	XMStoreFloat4x4(&m_Proj, XMMatrixIdentity());

	m_LastMouseX = 0;
	m_LastMouseY = 0;
}

OrbitCamera::~OrbitCamera()
{
}

bool OrbitCamera::Initialize()
{
	ID3D11Device* device = Direct3D_GetDevice();
	if (!device) return false;

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;

	if (FAILED(device->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferView)))
		return false;

	if (FAILED(device->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferProj)))
		return false;

	return true;
}

void OrbitCamera::Finalize()
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

void OrbitCamera::HandleKeyInput(double elapsed_time, bool enableRotation)
{
	if (!enableRotation)
	{
		return;
	}

	float dt = (float)elapsed_time;
	XMVECTOR position = XMLoadFloat3(&m_CameraPosition);

	// ---- ←/→ Key : Yaw ----
	if (KeyLogger_IsPressed(KEY_LEFT))
	{
		m_CameraYaw += CAMERA_ROTATE_SPEED_ORBIT * dt;
	}
	if (KeyLogger_IsPressed(KEY_RIGHT))
	{
		m_CameraYaw -= CAMERA_ROTATE_SPEED_ORBIT * dt;
	}
	m_CameraYaw = fmodf(m_CameraYaw, XM_2PI);
	if (m_CameraYaw < 0) m_CameraYaw += XM_2PI;

	// ---- ↑/↓ Key : Pitch ----
	if (KeyLogger_IsPressed(KEY_UP))
	{
		m_CameraPitch += CAMERA_ROTATE_SPEED_ORBIT * dt;
	}
	if (KeyLogger_IsPressed(KEY_DOWN))
	{
		m_CameraPitch -= CAMERA_ROTATE_SPEED_ORBIT * dt;
	}
	m_CameraPitch = std::max(MIN_PITCH, std::min(MAX_PITCH, m_CameraPitch));
}

void OrbitCamera::HandleMouseInput(double elapsed_time, const Mouse_State& mouseState)
{
	float dt = (float)elapsed_time;

	// ---- mouse scrolling to control zoom ----
	if (mouseState.scrollWheelValue != 0)
	{
		m_CameraRadius -= (float)mouseState.scrollWheelValue * CAMERA_ZOOM_SPEED;

		Mouse_ResetScrollWheelValue(); // reset mouse scroll value
	}
	m_CameraRadius = std::max(0.1f, m_CameraRadius);

	// ---- Mouse dragging control ----
	bool isAltPressed = KeyLogger_IsPressed(KK_LEFTALT) || KeyLogger_IsPressed(KK_RIGHTALT);
	bool isMiddleButtonPressed = mouseState.middleButton;
	bool isLeftButtonPressed = mouseState.leftButton;

	float mouse_dx = (float)(mouseState.x - m_LastMouseX);
	float mouse_dy = (float)(mouseState.y - m_LastMouseY);

	// Middle key to control panning
	if (isAltPressed && isMiddleButtonPressed)
	{

		XMVECTOR target = XMLoadFloat3(&m_CameraTarget);
		XMVECTOR position = XMLoadFloat3(&m_CameraPosition);

		// front vector : from camera to target
		XMVECTOR front = XMVector3Normalize(target - position);
		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		// right vector
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, front));

		// camera up vector
		XMVECTOR cameraUp = XMVector3Normalize(XMVector3Cross(front, right));

		float distance = XMVectorGetX(XMVector3Length(target - position));
		float pan_scale = CAMERA_PAN_SENSITIVITY * distance;

		// panning on  x axis : panning on right vector
		// panning on  y axis : panning on camera up vector
		XMVECTOR translationX = right * (mouse_dx * -pan_scale);
		XMVECTOR translationY = cameraUp * (mouse_dy * pan_scale);

		// also move target and camera position
		target += translationX + translationY;
		position += translationX + translationY;

		XMStoreFloat3(&m_CameraPosition, position);
		XMStoreFloat3(&m_CameraTarget, target);
	}

	// Left key to control rotate
	if (isAltPressed && isLeftButtonPressed)
	{
		m_CameraYaw += CAMERA_ROTATE_SENSITIVITY * mouse_dx;
		m_CameraYaw = fmodf(m_CameraYaw, XM_2PI);
		if (m_CameraYaw < 0) m_CameraYaw += XM_2PI;

		m_CameraPitch -= CAMERA_ROTATE_SENSITIVITY * mouse_dy;
		m_CameraPitch = std::max(MIN_PITCH, std::min(MAX_PITCH, m_CameraPitch));
	}

	m_LastMouseX = mouseState.x;
	m_LastMouseY = mouseState.y;
}

void OrbitCamera::Update(double elasped_time)
{
	// get mouse status
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);

	HandleKeyInput(elasped_time, m_KeyRotationEnabled);
	HandleMouseInput(elasped_time, mouseState);

	// recalculating camera position
	float sinPitch = sinf(m_CameraPitch);
	float cosPitch = cosf(m_CameraPitch);
	float sinYaw = sinf(m_CameraYaw);
	float cosYaw = cosf(m_CameraYaw);

	float offsetX = m_CameraRadius * sinPitch * sinYaw;
	float offsetY = m_CameraRadius * cosPitch;
	float offsetZ = m_CameraRadius * sinPitch * cosYaw;

	m_CameraPosition.x = m_CameraTarget.x + offsetX;
	m_CameraPosition.y = m_CameraTarget.y + offsetY;
	m_CameraPosition.z = m_CameraTarget.z + offsetZ;

	XMVECTOR position = XMLoadFloat3(&m_CameraPosition);
	XMVECTOR target = XMLoadFloat3(&m_CameraTarget);
	XMVECTOR up = XMLoadFloat3(&m_CameraUp);

	// ビュー座標変換行列の作成
	XMMATRIX mtxView = XMMatrixLookAtLH(
		position, //camera pos
		target, //focus point
		up//up dir
	);

	XMStoreFloat4x4(&m_View, mtxView);
	ID3D11DeviceContext* ctx = Direct3D_GetContext();
	XMFLOAT4X4 viewT;
	XMStoreFloat4x4(&viewT, XMMatrixTranspose(mtxView));
	ctx->UpdateSubresource(m_pVSConstantBufferView, 0, nullptr, &viewT, 0, 0);

	// Perspective array
	float fovAngleY = XMConvertToRadians(m_CameraFov);
	float aspectRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	float nearZ = 0.1f;
	float farZ = 100.0f;
	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);

	XMStoreFloat4x4(&m_Proj, mtxPerspective);
	XMFLOAT4X4 projT;
	XMStoreFloat4x4(&projT, XMMatrixTranspose(mtxPerspective));
	ctx->UpdateSubresource(m_pVSConstantBufferProj, 0, nullptr, &projT, 0, 0);
	
	// 定数バッファを描画パイプラインに設定
	ctx->VSSetConstantBuffers(1, 1, &m_pVSConstantBufferView);
	ctx->VSSetConstantBuffers(2, 1, &m_pVSConstantBufferProj);
}

XMFLOAT3 OrbitCamera::GetFront() const
{
	XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&m_CameraTarget) - XMLoadFloat3(&m_CameraPosition));
	XMFLOAT3 out{};
	XMStoreFloat3(&out, front);
	return out;
}

float OrbitCamera::GetFov()
{
	return m_CameraFov;
}


// Only for drawing itself
void OrbitCamera::DebugDraw()
{
	//ImGui::AlignTextToFramePadding();
	//ImGui::Text("--- Orbit Camera Debug ---");

	ImGui::Text("Position: %.2f, %.2f, %.2f", m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);

	ImGui::DragFloat3("Target", (float*)&m_CameraTarget, 0.1f);

	ImGui::SliderFloat("FOV ", &m_CameraFov, 10.f, 80.0f);
	ImGui::SliderFloat("Radius ", &m_CameraRadius, 0.1f, 50.0f);

	float yawDeg = XMConvertToDegrees(m_CameraYaw);
	if (ImGui::SliderFloat("Yaw ", &yawDeg, 0.0f, 360.0f))
	{
		m_CameraYaw = XMConvertToRadians(yawDeg);
	}

	float pitchDeg = XMConvertToDegrees(m_CameraPitch);
	if (ImGui::SliderFloat("Pitch ", &pitchDeg, 1.0f, 179.0f))
	{
		m_CameraPitch = XMConvertToRadians(pitchDeg);
	}

	ImGui::Separator();

	ImGui::Checkbox("Enable Keyboard Rotation", &m_KeyRotationEnabled);
}

