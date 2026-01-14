/*==============================================================================

   Axis FixÉwÉãÉpÅ[ [axis_util.cpp]
														 Author : Gu Anyi
														 Date   : 2025/12/10
--------------------------------------------------------------------------------

==============================================================================*/

#include "axis_util.h"

using namespace DirectX;

XMMATRIX GetAxisConversion(UpAxis source, UpAxis target)
{
	if (source == target)
		return XMMatrixIdentity();

	// Z-up to Y-up
	if (source == UpAxis::Z_Up && target == UpAxis::Y_Up)
		return XMMatrixRotationX(XM_PIDIV2);

	if (source == UpAxis::Y_Up && target == UpAxis::Z_Up)
		return XMMatrixRotationX(-XM_PIDIV2);

	return XMMatrixIdentity();
}

UpAxis UpFromBool(bool sourceYup)
{
	return sourceYup ? UpAxis::Y_Up : UpAxis::Z_Up;
}
