/*==============================================================================

   Outline描画専用シェーダ― [outline_shader.h]
														 Author : Gu Anyi
														 Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef OUTLINE_SHADER_H
#define OUTLINE_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

class OutlineShader
{
public:

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Finalize();

	void Begin();

	void SetParams(
		uint32_t selectedId,
		uint32_t width,
		uint32_t height,
		uint32_t thickness,
		const float color[4]
	);


private:

	// No need to release
	ID3D11Device*        m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// d3d11 resources
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	//ID3D11InputLayout* m_pInputLayout = nullptr;

	// constant buffer
	ID3D11Buffer* m_pConstantBuffer = nullptr;

	// states for outline
	ID3D11SamplerState*      m_SS_PointSampler = nullptr;
	ID3D11BlendState*        m_BS_AlphaBlend = nullptr;
	ID3D11DepthStencilState* m_DSS_NoDepth = nullptr;
	ID3D11RasterizerState*   m_RS = nullptr;

private:

	struct CBData
	{
		uint32_t selectedId;
		uint32_t width;
		uint32_t height;
		uint32_t thickness;
		float color[4];
	};
};


#endif // OUTLINE_SHADER_H