/*==============================================================================

   Outline描画専用シェーダ― [outline_shader.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/

#include <fstream>

#include "outline_shader.h"
#include "direct3d.h"
#include "debug_ostream.h"

using namespace DirectX;

bool OutlineShader::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr;

	m_pDevice = pDevice;
	m_pContext = pContext;

	// Vertex shaderの読み込み
	std::ifstream ifs_vs("shader_vertex_fullscreen.cso", std::ios::binary);
	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\shader_vertex_fullscreen.cso", "エラー", MB_OK);
		return false;
	}
	ifs_vs.seekg(0, std::ios::end);
	std::streamsize filesize = ifs_vs.tellg();
	ifs_vs.seekg(0, std::ios::beg);
	unsigned char* vsbinary_pointer = new unsigned char[filesize];
	ifs_vs.read((char*)vsbinary_pointer, filesize);
	ifs_vs.close();

	// 頂点シェーダーの作成
	hr = m_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &m_pVertexShader);
	if (FAILED(hr)) {
		hal::dout << "頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer;
		return false;
	}

	// Pixel shaderの読み込み
	std::ifstream ifs_ps("shader_pixel_outline_post.cso", std::ios::binary);
	if (!ifs_ps)
	{
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\shader_pixel_outline_post.cso", "エラー", MB_OK);
		return false;
	}
	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);
	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = m_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &m_pPixelShader);
	delete[] psbinary_pointer;

	if (FAILED(hr))
	{
		hal::dout << "ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	// Constant buffer
	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(CBData);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
		if (FAILED(hr)) return false;
	}

	// States
	// Point Sampler
	{
		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		
		hr = m_pDevice->CreateSamplerState(&sd, &m_SS_PointSampler);
		if (FAILED(hr)) return false;
	}
	 
	// RS
	{
		D3D11_RASTERIZER_DESC rd{};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = TRUE;

		hr = m_pDevice->CreateRasterizerState(&rd, &m_RS);
		if (FAILED(hr)) return false;
	}
	// DSS
	{
		D3D11_DEPTH_STENCIL_DESC dd{};
		dd.DepthEnable = FALSE;
		dd.StencilEnable = FALSE;

		hr = m_pDevice->CreateDepthStencilState(&dd, &m_DSS_NoDepth);
		if (FAILED(hr)) return false;
	}
	// BS
	{
		D3D11_BLEND_DESC bd{};
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		
		hr = m_pDevice->CreateBlendState(&bd, &m_BS_AlphaBlend);
		if (FAILED(hr)) return false;
	}

	return true;
}

void OutlineShader::Finalize()
{
	SAFE_RELEASE(m_SS_PointSampler);
	SAFE_RELEASE(m_BS_AlphaBlend)
	SAFE_RELEASE(m_DSS_NoDepth);
	SAFE_RELEASE(m_RS);

	SAFE_RELEASE(m_pConstantBuffer);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);
}

void OutlineShader::Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);
	m_pContext->IASetInputLayout(nullptr);
	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 定数バッファを描画パイプラインに設定
	m_pContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	m_pContext->PSSetSamplers(0, 1, &m_SS_PointSampler);
	m_pContext->OMSetBlendState(m_BS_AlphaBlend, nullptr, 0xffffffff);
	m_pContext->OMSetDepthStencilState(m_DSS_NoDepth, 0);
	m_pContext->RSSetState(m_RS);
}

void OutlineShader::SetParams(
	uint32_t selectedId,
	uint32_t width,
	uint32_t height,
	uint32_t thickness,
	const float color[4]
)
{
	D3D11_MAPPED_SUBRESOURCE ms{};
	if (SUCCEEDED(m_pContext->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)) && ms.pData)
	{
		CBData* cb = (CBData*)ms.pData;
		cb->selectedId = selectedId;
		cb->width = width;
		cb->height = height;
		cb->thickness = (thickness == 0) ? 1 : thickness;
		cb->color[0] = color[0];
		cb->color[1] = color[1];
		cb->color[2] = color[2];
		cb->color[3] = color[3];
		m_pContext->Unmap(m_pConstantBuffer, 0);
	}
}
