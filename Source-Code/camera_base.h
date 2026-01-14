/*==============================================================================

   Camera base class API [camera_base.h]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef CAMERA_BASE_H
#define CAMERA_BASE_H

#include <DirectXMath.h>

//struct Mouse_State;

class CameraBase
{
public:

	virtual ~CameraBase() = default;
	virtual void Update(double elapsed_time) = 0;

	virtual const DirectX::XMFLOAT4X4& GetView() const = 0;
	virtual const DirectX::XMFLOAT4X4& GetProj() const = 0;
	virtual const DirectX::XMFLOAT3& GetPosition() const = 0;
	virtual DirectX::XMFLOAT3 GetFront() const = 0;

};

#endif // CAMERA_BASE_H
