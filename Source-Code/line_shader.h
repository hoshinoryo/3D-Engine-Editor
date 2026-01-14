/*==============================================================================

   Line shader [line_shader.h]
														 Author : Gu Anyi
														 Date   : 2025/12/17
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef LINE_SHADER_H
#define	LINE_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

class LineShader
{
public:

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Finalize();

	void Begin();
	void SetWorldMatrix(const DirectX::XMMATRIX& mtxWorld);
	//void SetViewProjBuffers(ID3D11Buffer* view, ID3D11Buffer* proj);

private:

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// d3d11 resources
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;

	ID3D11Buffer* m_pVSConstantBufferWorld = nullptr; // matrix for local to world(b0)
	//ID3D11Buffer* m_pVSConstantBufferView = nullptr; // matrix for local to view(b1)
	//ID3D11Buffer* m_pVSConstantBufferProj = nullptr; // matrix for local to proj(b2)
};

#endif // LINE_SHADER_H