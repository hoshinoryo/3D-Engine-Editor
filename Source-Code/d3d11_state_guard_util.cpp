/*==============================================================================

   D3D11 states save and restore [d3d11_state_guard_util.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/08
--------------------------------------------------------------------------------

==============================================================================*/

#include "d3d11_state_guard_util.h"
#include "direct3d.h"

#include <cstring>


// Save states
void D3D11StateGuard::Begin(ID3D11DeviceContext* pContext, uint32_t mask)
{
	if (m_Active) End();

	m_pContext = pContext;
	m_Mask = mask;
	m_Active = (m_pContext != nullptr);

	if (!m_Active) return;

	// OM: RS/DT
	if (m_Mask & RenderTargets)
	{
		std::memset(m_OldRTVs, 0, sizeof(m_OldRTVs));
		m_OldDSV = nullptr;

		m_pContext->OMGetRenderTargets(
			D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
			m_OldRTVs,
			&m_OldDSV
		);

		m_OldNumRT = CalcBoundCountRTV(m_OldRTVs, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	}

	// OM: Blend/DepthStencil
	if (m_Mask & BlendStates)   m_pContext->OMGetBlendState(&m_OldBS, m_OldBlendFactor, &m_OldSampleMask);
	if (m_Mask & DepthStencil)  m_pContext->OMGetDepthStencilState(&m_OldDSS, &m_OldStencilRef);

	// RS: Rasterizer/Viewports/Scissors
	if (m_Mask & Rasterizer)    m_pContext->RSGetState(&m_OldRS);
	if (m_Mask & Viewports)
	{
		m_OldNumVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		std::memset(m_OldVPs, 0, sizeof(m_OldVPs));
		m_pContext->RSGetViewports(&m_OldNumVP, m_OldVPs);
	}
	if (m_Mask & Scissors)
	{
		m_OldNumScissors = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		std::memset(m_OldScissors, 0, sizeof(m_OldScissors));
		m_pContext->RSGetScissorRects(&m_OldNumScissors, m_OldScissors);
	}

	// IA: Topology/InputLayout/Buffer
	if (m_Mask & Topology)      m_pContext->IAGetPrimitiveTopology(&m_OldTopo);
	if (m_Mask & InputLayout)   m_pContext->IAGetInputLayout(&m_OldIL);
	if (m_Mask & IABuffers)
	{
		std::memset(m_OldVBs, 0, sizeof(m_OldVBs));
		std::memset(m_OldVBStrides, 0, sizeof(m_OldVBStrides));
		std::memset(m_OldVBOffsets, 0, sizeof(m_OldVBOffsets));

		m_pContext->IAGetVertexBuffers(
			0,
			D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
			m_OldVBs,
			m_OldVBStrides,
			m_OldVBOffsets
		);

		m_OldIB = nullptr;
		m_OldIBFormat = DXGI_FORMAT_UNKNOWN;
		m_OldIBOffset = 0;
		m_pContext->IAGetIndexBuffer(&m_OldIB, &m_OldIBFormat, &m_OldIBOffset);
	}

	// Shader
	if (m_Mask & Shaders)
	{
		m_pContext->VSGetShader(&m_OldVS, nullptr, nullptr);
		m_pContext->PSGetShader(&m_OldPS, nullptr, nullptr);
	}
	
	// PS SRV0
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

	if (m_Mask & IABuffers)
	{
		m_pContext->IASetVertexBuffers(
			0,
			D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
			m_OldVBs,
			m_OldVBStrides,
			m_OldVBOffsets
		);

		m_pContext->IASetIndexBuffer(m_OldIB, m_OldIBFormat, m_OldIBOffset);
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

	if (m_Mask & Scissors)
	{
		if (m_OldNumScissors > 0)
		{
			m_pContext->RSSetScissorRects(m_OldNumScissors, m_OldScissors);
		}
	}
	if (m_Mask & Rasterizer) m_pContext->RSSetState(m_OldRS);
	if (m_Mask & Viewports)
	{
		if (m_OldNumVP > 0)
		{
			m_pContext->RSSetViewports(m_OldNumVP, m_OldVPs);
		}
	}

	if (m_Mask & RenderTargets)
	{
		if (m_OldNumRT == 0)
		{
			m_pContext->OMSetRenderTargets(0, nullptr, m_OldDSV);
		}
		else
		{
			m_pContext->OMSetRenderTargets(m_OldNumRT, m_OldRTVs, m_OldDSV);
		}
	}
	if (m_Mask & PS_SRV0) m_pContext->PSSetShaderResources(0, 1, &m_OldSRV0);

	// Release
	if (m_Mask & RenderTargets)
	{
		for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		{
			SAFE_RELEASE(m_OldRTVs[i]);
		}
		SAFE_RELEASE(m_OldDSV);
		m_OldNumRT = 0;
	}
	SAFE_RELEASE(m_OldBS);
	SAFE_RELEASE(m_OldDSS);
	SAFE_RELEASE(m_OldRS);
	SAFE_RELEASE(m_OldIL);
	SAFE_RELEASE(m_OldVS);
	SAFE_RELEASE(m_OldPS);
	SAFE_RELEASE(m_OldSRV0);

	if (m_Mask & IABuffers)
	{
		for (UINT i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++)
		{
			SAFE_RELEASE(m_OldVBs[i]);
		}
		SAFE_RELEASE(m_OldIB);
		m_OldIBFormat = DXGI_FORMAT_UNKNOWN;
		m_OldIBOffset = 0;
	}

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

UINT D3D11StateGuard::CalcBoundCountRTV(ID3D11RenderTargetView* const* rtvs, UINT maxCount)
{
	for (int i = (int)maxCount - 1; i >= 0; i--)
	{
		if (rtvs[i] != nullptr)
		{
			return (UINT)(i + 1);
		}
	}
	return 0;
}

/*
	D3D11StateGuard

	Purpose:
		RAII-style helper to save and restore D3D11 device context states.

	How it works:
		- Begin(): query current states from ID3D11DeviceContext
		- Store them based on bit flag
		- End(): restore all stored states in reverse-safe order

	Supported states:
		- OM : RenderTargets, BlendState, DepthStencilState
		- RS : RasterizerState, Viewports, ScissorRects
		- IA : PrimitiveTopology, InputLayout, VertexBuffer / IndexBuffer
		- Shaders : VS / PS
		- PS SRV slot0

	Typical usage:
		{
			D3D11StateGuard guard(ctx,
				D3D11StateGuard::IABuffers |
				D3D11StateGuard::Topology |
				D3D11StateGuard::Shaders
			);

			// do custom draw here
		}
		// all D3D states restored automatically
*/