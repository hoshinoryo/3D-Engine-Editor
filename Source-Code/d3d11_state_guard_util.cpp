/*==============================================================================

   D3D11 states save and restore [d3d11_state_guard_util.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/08
--------------------------------------------------------------------------------

==============================================================================*/

#include "d3d11_state_guard_util.h"
#include "direct3d.h"

// Save states
void D3D11StateGuard::Begin(ID3D11DeviceContext* pContext, uint32_t mask)
{
	if (m_Active) End();

	m_pContext = pContext;
	m_Mask = mask;
	m_Active = (m_pContext != nullptr);

	if (!m_Active) return;

	if (m_Mask & RenderTargets) m_pContext->OMGetRenderTargets(1, &m_OldRTV, &m_OldDSV);
	if (m_Mask & BlendStates)   m_pContext->OMGetBlendState(&m_OldBS, m_OldBlendFactor, &m_OldSampleMask);
	if (m_Mask & DepthStencil)  m_pContext->OMGetDepthStencilState(&m_OldDSS, &m_OldStencilRef);
	if (m_Mask & Rasterizer)    m_pContext->RSGetState(&m_OldRS);
	if (m_Mask & Viewports)
	{
		m_OldNumVP = 1;
		m_pContext->RSGetViewports(&m_OldNumVP, &m_OldVP);
	}
	if (m_Mask & Topology)      m_pContext->IAGetPrimitiveTopology(&m_OldTopo);
	if (m_Mask & InputLayout)   m_pContext->IAGetInputLayout(&m_OldIL);
	if (m_Mask & Shaders)
	{
		m_pContext->VSGetShader(&m_OldVS, nullptr, nullptr);
		m_pContext->PSGetShader(&m_OldPS, nullptr, nullptr);
	}
	if (m_Mask & PS_SRV0)       m_pContext->PSGetShaderResources(0, 1, &m_OldSRV0);
}

// Restore states
void D3D11StateGuard::End()
{
	if (!m_Active) return;

	if (m_Mask & Shaders)
	{
		m_pContext->VSSetShader(m_OldVS, nullptr, 0);
		m_pContext->PSSetShader(m_OldPS, nullptr, 0);
	}
	if (m_Mask & InputLayout) m_pContext->IASetInputLayout(m_OldIL);
	if (m_Mask & Topology)
	{
		if (m_OldTopo != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			m_pContext->IASetPrimitiveTopology(m_OldTopo);
		}
	}
	if (m_Mask & BlendStates) m_pContext->OMSetBlendState(m_OldBS, m_OldBlendFactor, m_OldSampleMask);
	if (m_Mask & DepthStencil) m_pContext->OMSetDepthStencilState(m_OldDSS, m_OldStencilRef);
	if (m_Mask & Rasterizer) m_pContext->RSSetState(m_OldRS);
	if (m_Mask & Viewports)
	{
		if (m_OldNumVP > 0)
		{
			m_pContext->RSSetViewports(m_OldNumVP, &m_OldVP);
		}
	}
	if (m_Mask & RenderTargets) m_pContext->OMSetRenderTargets(1, &m_OldRTV, m_OldDSV);
	if (m_Mask & PS_SRV0) m_pContext->PSSetShaderResources(0, 1, &m_OldSRV0);

	// Release
	SAFE_RELEASE(m_OldRTV);
	SAFE_RELEASE(m_OldDSV);
	SAFE_RELEASE(m_OldBS);
	SAFE_RELEASE(m_OldDSS);
	SAFE_RELEASE(m_OldRS);
	SAFE_RELEASE(m_OldIL);
	SAFE_RELEASE(m_OldVS);
	SAFE_RELEASE(m_OldPS);
	SAFE_RELEASE(m_OldSRV0);

	m_pContext = nullptr;
	m_Mask = 0;
	m_Active = false;
}

void D3D11StateGuard::UnbindPSSRV0()
{
	if (!m_Active) return;

	ID3D11ShaderResourceView* nullSRV = nullptr;
	m_pContext->PSSetShaderResources(0, 1, &nullSRV);
}
