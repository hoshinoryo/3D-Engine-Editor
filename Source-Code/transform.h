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
	DirectX::XMFLOAT3 position{ 0, 0, 0 };
	DirectX::XMFLOAT3 rotationDeg{ 0, 0, 0 };
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };

	DirectX::XMMATRIX ToMatrix() const
	{
		const float pitch = DirectX::XMConvertToRadians(rotationDeg.x);
		const float yaw = DirectX::XMConvertToRadians(rotationDeg.y);
		const float roll = DirectX::XMConvertToRadians(rotationDeg.z);

		const DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		const DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
		const DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		return S * R * T; // DO NOT TOUCH!!
	}
};

#endif // TRANSFORM_H

