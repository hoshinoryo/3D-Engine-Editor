/*==============================================================================

   Axis FixÉwÉãÉpÅ[ [axis_util.h]
														 Author : Gu Anyi
														 Date   : 2025/12/10
--------------------------------------------------------------------------------

==============================================================================*/

#include <DirectXMath.h>

enum class UpAxis
{
	Y_Up,
	Z_Up
};

DirectX::XMMATRIX GetAxisConversion(UpAxis source, UpAxis target);
UpAxis UpFromBool(bool sourceYup);