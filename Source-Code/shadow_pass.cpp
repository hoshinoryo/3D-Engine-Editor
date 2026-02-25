/*==============================================================================

   âeÇÃï`âÊä«óù [shadow_pass.cpp]
                                                         Author : Gu Anyi
                                                         Date   : 2026/02/17

--------------------------------------------------------------------------------

==============================================================================*/

//#pragma message("COMPILING shadow_pass.cpp in this project")

#include <cstring>

#include "shadow_pass.h"
#include "direct3d.h"
#include "sampler.h"

using namespace DirectX;

ShadowPass g_ShadowPass;

bool ShadowPass::Initialize(ID3D11Device* device, int shadowMapSize, UINT shadowSRVSlot)
{
    m_Size = shadowMapSize;
    m_ShadowSRVSlot = shadowSRVSlot;

    XMStoreFloat4x4(&m_LightViewProj, XMMatrixIdentity());

    if (!CreateShadowMapResources(device)) return false;

    // viewport
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = (float)m_Size;
    m_Viewport.Height = (float)m_Size;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    D3D11_BUFFER_DESC desc = { sizeof(ShadowConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (FAILED(device->CreateBuffer(&desc, nullptr, &m_pShadowCB))) return false;

    return true;
}

void ShadowPass::Finalize()
{
    SAFE_RELEASE(m_pShadowCB);
    SAFE_RELEASE(m_OldShadowSRV);
    
    SAFE_RELEASE(m_ShadowSRV);
    SAFE_RELEASE(m_ShadowDSV);
    SAFE_RELEASE(m_ShadowTex);
}

void ShadowPass::Begin(ID3D11DeviceContext* ctx)
{
    m_Guard.Begin(ctx,
        D3D11StateGuard::RenderTargets |
        D3D11StateGuard::Viewports
    );

    SAFE_RELEASE(m_OldShadowSRV);
    ctx->PSGetShaderResources(m_ShadowSRVSlot, 1, &m_OldShadowSRV);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(m_ShadowSRVSlot, 1, &nullSRV);

    // shadow viewport + DSV
    ctx->RSSetViewports(1, &m_Viewport);
    ctx->OMSetRenderTargets(0, nullptr, m_ShadowDSV);

    // clear depth
    ctx->ClearDepthStencilView(m_ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void ShadowPass::End(ID3D11DeviceContext* ctx)
{
    if (!ctx) return;

    ctx->PSSetShaderResources(m_ShadowSRVSlot, 1, &m_OldShadowSRV);

    SAFE_RELEASE(m_OldShadowSRV);

    m_Guard.End();
}

void ShadowPass::SetLightViewProj(const DirectX::XMFLOAT4X4& lightViewProj)
{
    m_LightViewProj = lightViewProj;
}

void ShadowPass::BindShadowMapSRV(ID3D11DeviceContext* ctx) const
{
    ctx->PSSetShaderResources(m_ShadowSRVSlot, 1, &m_ShadowSRV);
}

void ShadowPass::PrepareForMainPass(ID3D11DeviceContext* ctx)
{
    if (!m_pShadowCB) return;

    XMMATRIX V = XMLoadFloat4x4(&m_LightCam.GetViewRaw());
    XMMATRIX P = XMLoadFloat4x4(&m_LightCam.GetProjRaw());

    XMMATRIX LVP = V * P;

    ShadowConstantBuffer data;
    XMStoreFloat4x4(&data.lightViewProj, XMMatrixTranspose(LVP));
    data.shadowBias      = 0.0025f;
    data.shadowStrength  = 0.7f;
    data.shadowTexelSize = { 1.0f / m_Size, 1.0f / m_Size };

    ctx->UpdateSubresource(m_pShadowCB, 0, nullptr, &data, 0, 0);

    // binding
    ctx->VSSetConstantBuffers(5, 1, &m_pShadowCB);
    ctx->PSSetConstantBuffers(5, 1, &m_pShadowCB);

    this->BindShadowMapSRV(ctx);

    Sampler_SetShadowCompare();
}

void ShadowPass::CleanUpAfterMainPass(ID3D11DeviceContext* ctx)
{
    Sampler_RestoreShadowSlot(); // restore sampler
}

bool ShadowPass::CreateShadowMapResources(ID3D11Device* device)
{
    D3D11_TEXTURE2D_DESC td{};
    td.Width = m_Size;
    td.Height = m_Size;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device->CreateTexture2D(&td, nullptr, &m_ShadowTex))) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
    dsvd.Format = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvd.Texture2D.MipSlice = 0;

    if (FAILED(device->CreateDepthStencilView(m_ShadowTex, &dsvd, &m_ShadowDSV))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    srvd.Texture2D.MostDetailedMip = 0;

    if (FAILED(device->CreateShaderResourceView(m_ShadowTex, &srvd, &m_ShadowSRV))) return false;

    return true;
}

bool ShadowPass::LightCamera::Initialize(ID3D11Device* device)
{
    if (!device) return false;

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(XMFLOAT4X4);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    if (FAILED(device->CreateBuffer(&bd, nullptr, &m_pVSConstantBufferView))) return false;
    if (FAILED(device->CreateBuffer(&bd, nullptr, &m_pVSConstantBufferProj))) return false;

    XMStoreFloat4x4(&m_View, XMMatrixIdentity());
    XMStoreFloat4x4(&m_Proj, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ViewT, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ProjT, XMMatrixIdentity());

    return true;
}

void ShadowPass::LightCamera::Finalize() 
{
    SAFE_RELEASE(m_pVSConstantBufferView);
    SAFE_RELEASE(m_pVSConstantBufferProj);
}

static XMFLOAT3 Normalize3(const XMFLOAT3& v)
{
    XMVECTOR vv = XMLoadFloat3(&v);
    vv = XMVector3Normalize(vv);
    XMFLOAT3 o;
    XMStoreFloat3(&o, vv);
    return o;
}

void ShadowPass::LightCamera::UpdateDirectional(
    const XMFLOAT3& dirW,
    const XMFLOAT3& targetW,
    float distance,
    float orthoW, float orthoH,
    float nearZ, float farZ
)
{
    XMFLOAT3 d = Normalize3(dirW);

    m_Target = targetW;
    m_Position = { targetW.x - d.x * distance,
                   targetW.y - d.y * distance,
                   targetW.z - d.z * distance };

    XMVECTOR pos = XMLoadFloat3(&m_Position);
    XMVECTOR tgt = XMLoadFloat3(&m_Target);

    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR vDir = XMVector3Normalize(tgt - pos);
    float dp = XMVectorGetX(XMVectorAbs(XMVector3Dot(vDir, up)));
    if (dp > 0.99f) up = XMVectorSet(0, 0, 1, 0);

    XMMATRIX view = XMMatrixLookAtLH(pos, tgt, up);
    XMMATRIX proj = XMMatrixOrthographicLH(orthoW, orthoH, nearZ, farZ);

    XMStoreFloat4x4(&m_View, view);
    XMStoreFloat4x4(&m_Proj, proj);

    XMStoreFloat4x4(&m_ViewT, XMMatrixTranspose(view));
    XMStoreFloat4x4(&m_ProjT, XMMatrixTranspose(proj));
}

void ShadowPass::LightCamera::BindToVS(ID3D11DeviceContext* ctx, UINT viewSlot, UINT projSlot) const
{
    if (!ctx) return;

    ctx->VSSetConstantBuffers(viewSlot, 1, &m_pVSConstantBufferView);
    ctx->VSSetConstantBuffers(projSlot, 1, &m_pVSConstantBufferProj);
}

void ShadowPass::LightCamera::Upload()
{
    ID3D11DeviceContext* ctx = Direct3D_GetContext();
    if (!ctx) return;

    ctx->UpdateSubresource(m_pVSConstantBufferView, 0, nullptr, &m_ViewT, 0, 0);
    ctx->UpdateSubresource(m_pVSConstantBufferProj, 0, nullptr, &m_ProjT, 0, 0);
}


