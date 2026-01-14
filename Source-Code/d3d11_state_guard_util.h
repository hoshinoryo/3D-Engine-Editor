/*==============================================================================

   D3D11 states save and restore [d3d11_state_guard_util.h]
														 Author : Gu Anyi
														 Date   : 2026/01/08
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef D3D11_STATE_GUARD_UTIL_H
#define D3D11_STATE_GUARD_UTIL_H

#include <d3d11.h>
#include <cstdint>

class D3D11StateGuard
{
public:

	// Bit flag can make several masks active at the same time
	enum Mask : uint32_t
	{
		None          = 0,
		RenderTargets = 1u << 0, // OMGetRenderTargets / OMSetRenderTargets
		BlendStates   = 1u << 1, // OMGetBlendState
		DepthStencil  = 1u << 2, // OMGetDepthStencilState
		Rasterizer    = 1u << 3, // RSGetState
		Viewports     = 1u << 4, // RSGetViewports
		Topology      = 1u << 5, // IAGetPrimitiveTopology
		InputLayout   = 1u << 6, // IAGetInoutLayout
		Shaders       = 1u << 7, // VSGetShader / PSGetShader
		PS_SRV0       = 1u << 8, // PSGetShaderResources (slot0)
		All           = 0xFFFFFFFFu
	};

	D3D11StateGuard() = default;
	D3D11StateGuard(ID3D11DeviceContext* pContext, uint32_t mask) { Begin(pContext, mask); };

	~D3D11StateGuard() { End(); }

	D3D11StateGuard(const D3D11StateGuard&) = delete;
	D3D11StateGuard& operator=(const D3D11StateGuard&) = delete;

	void Begin(ID3D11DeviceContext* pContext, uint32_t mask);
	void End();

	void UnbindPSSRV0();

private:

	ID3D11DeviceContext* m_pContext = nullptr;
	uint32_t m_Mask = 0;
	bool m_Active = false;

	// OM RT/DS
	ID3D11RenderTargetView* m_OldRTV = nullptr;
	ID3D11DepthStencilView* m_OldDSV = nullptr;

	// Blend
	ID3D11BlendState* m_OldBS = nullptr;
	float m_OldBlendFactor[4]{};
	UINT  m_OldSampleMask = 0xffffffff;

	// DSS
	ID3D11DepthStencilState* m_OldDSS = nullptr;
	UINT m_OldStencilRef = 0;

	// RS
	ID3D11RasterizerState* m_OldRS = nullptr;

	// Viewport
	D3D11_VIEWPORT m_OldVP{};
	UINT m_OldNumVP = 1;

	// IA
	ID3D11InputLayout* m_OldIL = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY m_OldTopo = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;

	// Shaders
	ID3D11VertexShader* m_OldVS = nullptr;
	ID3D11PixelShader*  m_OldPS = nullptr;

	// PS SRV slot0
	ID3D11ShaderResourceView* m_OldSRV0 = nullptr;

};

#endif // D3D11_STATE_GUARD_UTIL_H
