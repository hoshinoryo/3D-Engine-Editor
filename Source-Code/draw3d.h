/*==============================================================================

   Debug drawing [draw3d.h]
														 Author : Gu Anyi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DRAW3D_H
#define DRAW3D_H

#include <d3d11.h>
#include <DirectXMath.h>


void Draw3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Draw3d_Finalize(void);
void Draw3d_Draw(void);

void Draw3d_MakeLine(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT4& color);
void Draw3d_MakeCross(const DirectX::XMFLOAT3& center, float size, const DirectX::XMFLOAT4& color);

void Draw3d_MakeWireBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& halfSize, const DirectX::XMFLOAT4& color);
void Draw3d_MakeWireSphere(const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4& color, int segments = 32);

#endif // DRAW3D_H
