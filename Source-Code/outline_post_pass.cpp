/*==============================================================================

   Outline drawing [outline_post_pass.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2026/01/08
--------------------------------------------------------------------------------

==============================================================================*/

#include "outline_post_pass.h"
#include "direct3d.h"
#include "d3d11_state_guard_util.h"

#include <vector>
#include <fstream>
#include <cassert>


bool OutlinePostPass::Initialize(uint32_t width, uint32_t height)
{
    m_pDevice = Direct3D_GetDevice();
    m_pContext = Direct3D_GetContext();
    assert(m_pDevice && m_pContext);

    m_Width = width;
    m_Height = height;

    if (!m_OutlineShader.Initialize(m_pDevice, m_pContext)) return false;

    return true;
}

void OutlinePostPass::Finalize()
{
    m_OutlineShader.Finalize();

    m_pDevice = nullptr;
    m_pContext = nullptr;
}

void OutlinePostPass::Resize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
}

void OutlinePostPass::DrawModel(
    ID3D11ShaderResourceView* idSRV,
    uint32_t selectedId,
    uint32_t thickness,
    const float color[4]
)
{
    if (!m_pContext || !idSRV || selectedId == 0) return;

    D3D11StateGuard guard(m_pContext,
        D3D11StateGuard::Shaders |
        D3D11StateGuard::InputLayout |
        D3D11StateGuard::Topology |
        D3D11StateGuard::BlendStates |
        D3D11StateGuard::DepthStencil |
        D3D11StateGuard::Rasterizer |
        D3D11StateGuard::PS_SRV0
    );

    m_OutlineShader.SetParams(selectedId, m_Width, m_Height, thickness, color);
    m_OutlineShader.Begin();

    m_pContext->PSSetShaderResources(0, 1, &idSRV);

    // Draw full screen triangle
    m_pContext->Draw(3, 0);

    guard.UnbindPSSRV0();
}

