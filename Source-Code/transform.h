/*==============================================================================

   Model transform struction [transform.h]
														 Author : Gu Anyi
														 Date   : 2026/01/23
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <DirectXMath.h>

struct TransformTRS
{
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotationQuat{ 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

	DirectX::XMMATRIX ToMatrix() const
	{
		//const float pitch = DirectX::XMConvertToRadians(rotationQuat.x);
		//const float yaw = DirectX::XMConvertToRadians(rotationQuat.y);
		//const float roll = DirectX::XMConvertToRadians(rotationQuat.z);

		const DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		const DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotationQuat));
		const DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		return S * R * T; // DO NOT TOUCH!!
	}
};

#endif // TRANSFORM_H

