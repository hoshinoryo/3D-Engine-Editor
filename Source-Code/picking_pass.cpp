/*==============================================================================

   Offscreen rendering and object id management [picking_pass.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/

#include <cassert>
#include <vector>
#include <cstring>

#include "picking_pass.h"
#include "picking_shader.h"
#include "direct3d.h"
//#include "model.h"
#include "model_asset.h"
//#include "outliner.h"
#include "axis_util.h"

using namespace DirectX;

template<typename T>
static T ClampT(T v, T lo, T hi);


bool PickingPass::Initialize(uint32_t width, uint32_t height)
{
    m_pDevice = Direct3D_GetDevice();
    m_pContext = Direct3D_GetContext();
    assert(m_pDevice && m_pContext);

    m_Width = width;
    m_Height = height;

    if (!m_PickingShader.Initialize(m_pDevice, m_pContext)) return false;
    if (!CreateFixedStates()) return false;
    if (!CreateTargets(width, height)) return false;
    if (!CreateReadback()) return false;

    return true;
}

void PickingPass::Finalize()
{
    ReleaseTargets();
    ReleaseReadback();
    ReleaseFixedStates();
    
    m_PickingShader.Finalize();

    m_pDevice = nullptr;
    m_pContext = nullptr;
}

bool PickingPass::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) return false;

    m_Width = width;
    m_Height = height;

    return CreateTargets(width, height);
}

void PickingPass::Begin(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj)
{
    assert(m_pContext);

    m_View = view;
    m_Proj = proj;

    m_StateGuard.Begin(m_pContext,
        D3D11StateGuard::RenderTargets |
        D3D11StateGuard::BlendStates |
        D3D11StateGuard::Rasterizer |
        D3D11StateGuard::DepthStencil |
        D3D11StateGuard::Viewports |
        D3D11StateGuard::Topology
    );

    // Set picking targets
    m_pContext->OMSetRenderTargets(1, &m_IdRTV, m_DepthDSV);
    m_pContext->RSSetViewports(1, &m_VP);

    // Clear: ID = 0, depth = 1
    const float clear[4] = { 0, 0, 0, 0 };
    m_pContext->ClearRenderTargetView(m_IdRTV, clear);
    m_pContext->ClearDepthStencilView(m_DepthDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Force safe states for integer RT
    m_pContext->OMSetBlendState(m_BS_NoBlend_WriteAll, nullptr, 0xffffffff);
    m_pContext->RSSetState(m_RS_NoCull_NoScissor);
    m_pContext->OMSetDepthStencilState(m_DSS_DepthLessEqual_NoStencil, 0);

    // Shader
    m_PickingShader.Begin();
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void PickingPass::End()
{
    assert(m_pContext);

    m_StateGuard.End();
}

// Rendering (mesh-level)
void PickingPass::DrawAsset(ModelAsset* asset, uint32_t meshIndex, const DirectX::XMMATRIX& world, uint32_t objectId)
{
    if (!asset || !asset->aiScene) return;
    if (meshIndex >= asset->meshes.size()) return;
    assert(m_pContext);

    //XMMATRIX axisFix = GetAxisConversion(UpFromBool(asset->sourceYup), UpAxis::Y_Up);
    //XMMATRIX importScale = XMMatrixScaling(asset->importScale, asset->importScale, asset->importScale);
    //XMMATRIX finalWorld = importScale * axisFix * world;
    const XMMATRIX finalWorld = asset->importFix * world;

    m_PickingShader.SetParams(finalWorld, m_View, m_Proj, objectId);

    MeshAsset& mesh = asset->meshes[meshIndex];

    UINT stride = sizeof(Vertex3d);
    UINT offset = 0;

    m_pContext->IASetVertexBuffers(0, 1, &mesh.vertexBuffer, &stride, &offset);
    m_pContext->IASetIndexBuffer(mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    m_pContext->DrawIndexed(mesh.indexCount, 0, 0);
}

// From mouse coordinate to return object id
uint32_t PickingPass::ReadBackId(int mouseX, int mouseY)
{
    if (!m_IdTex || !m_ReadBack1x1) return 0;
    assert(m_pContext);

    // Clamp
    mouseX = ClampT(mouseX, 0, (int)m_Width - 1);
    mouseY = ClampT(mouseY, 0, (int)m_Height - 1);

    D3D11_BOX srcBox{};
    srcBox.left   = (UINT)mouseX;
    srcBox.right  = (UINT)mouseX + 1;
    srcBox.top    = (UINT)mouseY;
    srcBox.bottom = (UINT)mouseY + 1;
    srcBox.front  = 0;
    srcBox.back   = 1;

    m_pContext->CopySubresourceRegion(m_ReadBack1x1, 0, 0, 0, 0, m_IdTex, 0, &srcBox);

    // Map staging
    D3D11_MAPPED_SUBRESOURCE ms{};
    HRESULT hr = m_pContext->Map(m_ReadBack1x1, 0, D3D11_MAP_READ, 0, &ms);
    if (FAILED(hr)) return 0;

    uint32_t id = *reinterpret_cast<uint32_t*>(ms.pData);
    m_pContext->Unmap(m_ReadBack1x1, 0);

    return id;
}

bool PickingPass::CreateTargets(uint32_t width, uint32_t height)
{
    ReleaseTargets();

    assert(m_pDevice);

    // ID RT:R32_UINT
    {
        D3D11_TEXTURE2D_DESC td{};
        td.Width = width;
        td.Height = height;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_UINT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = m_pDevice->CreateTexture2D(&td, nullptr, &m_IdTex);
        if (FAILED(hr)) return false;

        // RTV
        D3D11_RENDER_TARGET_VIEW_DESC rvd{};
        rvd.Format = td.Format;
        rvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rvd.Texture2D.MipSlice = 0;

        hr = m_pDevice->CreateRenderTargetView(m_IdTex, &rvd, &m_IdRTV);
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC svd{};
        svd.Format = td.Format;
        svd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        svd.Texture2D.MipLevels = 1;
        svd.Texture2D.MostDetailedMip = 0;

        hr = m_pDevice->CreateShaderResourceView(m_IdTex, &svd, &m_IdSRV);
        if (FAILED(hr)) return false;
    }
    
    // Depth: D24S8
    {
        D3D11_TEXTURE2D_DESC depth_td{};
        depth_td.Width = width;
        depth_td.Height = height;
        depth_td.MipLevels = 1;
        depth_td.ArraySize = 1;
        depth_td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_td.SampleDesc.Count = 1;
        depth_td.Usage = D3D11_USAGE_DEFAULT;
        depth_td.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        HRESULT hr = m_pDevice->CreateTexture2D(&depth_td, nullptr, &m_DepthTex);
        if (FAILED(hr)) return false;

        hr = m_pDevice->CreateDepthStencilView(m_DepthTex, nullptr, &m_DepthDSV);
        if (FAILED(hr)) return false;
    }

    // viewpoint
    m_VP.TopLeftX = 0.0f;
    m_VP.TopLeftY = 0.0f;
    m_VP.Width = (float)width;
    m_VP.Height = (float)height;
    m_VP.MinDepth = 0.0f;
    m_VP.MaxDepth = 1.0F;
    
    return true;
}

void PickingPass::ReleaseTargets()
{
    SAFE_RELEASE(m_IdSRV);
    SAFE_RELEASE(m_IdRTV);
    SAFE_RELEASE(m_IdTex);
    SAFE_RELEASE(m_DepthDSV);
    SAFE_RELEASE(m_DepthTex);
}

bool PickingPass::CreateReadback()
{
    ReleaseReadback();
    assert(m_pDevice);

    D3D11_TEXTURE2D_DESC td{};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_UINT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    HRESULT hr = m_pDevice->CreateTexture2D(&td, nullptr, &m_ReadBack1x1);

    return SUCCEEDED(hr);
}

void PickingPass::ReleaseReadback()
{
    SAFE_RELEASE(m_ReadBack1x1);
}

bool PickingPass::CreateFixedStates()
{
    assert(m_pDevice);

    // Blend: disable blend + enable write work (IMPORTANT for R32_UINT RT)
    {
        D3D11_BLEND_DESC bd{};
        bd.RenderTarget[0].BlendEnable = FALSE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        HRESULT hr = m_pDevice->CreateBlendState(&bd, &m_BS_NoBlend_WriteAll);
        if (FAILED(hr)) return false;
    }

    // Rasterizer: no cull + depth clip
    {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        rd.ScissorEnable = FALSE;

        HRESULT hr = m_pDevice->CreateRasterizerState(&rd, &m_RS_NoCull_NoScissor);
        if (FAILED(hr)) return false;
    }

    // DepthStencil: depth on, write on, less equal, stencil off
    {
        D3D11_DEPTH_STENCIL_DESC dd{};
        dd.DepthEnable = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dd.StencilEnable = FALSE;

        HRESULT hr = m_pDevice->CreateDepthStencilState(&dd, &m_DSS_DepthLessEqual_NoStencil);
        if (FAILED(hr)) return false;
    }

    return true;
}

void PickingPass::ReleaseFixedStates()
{
    SAFE_RELEASE(m_BS_NoBlend_WriteAll);
    SAFE_RELEASE(m_RS_NoCull_NoScissor);
    SAFE_RELEASE(m_DSS_DepthLessEqual_NoStencil);
}

template<typename T>
T ClampT(T v, T lo, T hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}