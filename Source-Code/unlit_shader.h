/*==============================================================================

   Unlit shader [unlit_shader.h]
														 Author : Gu Anyi
														 Date   : 2025/12/29
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef UNLIT_SHADER_H
#define UNLIT_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

class UnlitShader
{
public:

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Finalize();

	void Begin();

	void SetWorldMatrix(const DirectX::XMMATRIX& mtxWorld);
	void SetColor(const DirectX::XMFLOAT4& color); // diffuse color

private:

	// No need to release
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// d3d11 resources
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;

	// VS constant buffer
	ID3D11Buffer* m_pVSConstantBufferWorld = nullptr; // matrix for local to world(b0)

	// PS constant buffer
	ID3D11Buffer* m_pPSConstantBuffer0 = nullptr; // diffuse color for pixel shader
};

extern UnlitShader g_DefaultUnlitShader;

#endif // UNLIT_SHADER_H
