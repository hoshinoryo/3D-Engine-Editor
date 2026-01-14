/*==============================================================================

   各ピクセルにobjectIdを書き込む用shader [picking_shader.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/05
--------------------------------------------------------------------------------

==============================================================================*/

#include <fstream>

#include "picking_shader.h"
#include "direct3d.h"
#include "debug_ostream.h"

using namespace DirectX;

bool PickingShader::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr;

	m_pDevice = pDevice;
	m_pContext = pContext;

	// Vertex shaderの読み込み
	std::ifstream ifs_vs("shader_vertex_picking.cso", std::ios::binary);
	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_picking.cso", "エラー", MB_OK);
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

	// 頂点レイアウト
	D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT num_elements = ARRAYSIZE(layout);
	hr = m_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &m_pInputLayout);
	delete[] vsbinary_pointer;
	if (FAILED(hr))
	{
		hal::dout << "頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}

	// Pixel shaderの読み込み
	std::ifstream ifs_ps("shader_pixel_picking.cso", std::ios::binary);
	if (!ifs_ps)
	{
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\shader_pixel_picking.cso", "エラー", MB_OK);
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
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(CBPicking);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pCBPicking);
	if (FAILED(hr)) return false;

	return true;
}

void PickingShader::Finalize()
{
	SAFE_RELEASE(m_pCBPicking);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);
}

void PickingShader::Begin()
{
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->VSSetConstantBuffers(0, 1, &m_pCBPicking);
	m_pContext->PSSetConstantBuffers(0, 1, &m_pCBPicking);
}

void PickingShader::SetParams(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj, uint32_t objectId)
{
	D3D11_MAPPED_SUBRESOURCE ms{};
	HRESULT hr = m_pContext->Map(m_pCBPicking, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr)) return;

	CBPicking* cb = (CBPicking*)ms.pData;

	XMStoreFloat4x4(&cb->world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&cb->view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&cb->proj, XMMatrixTranspose(proj));
	//XMStoreFloat4x4(&cb->view, view);
	//XMStoreFloat4x4(&cb->proj, proj);
	cb->objectId = objectId;

	m_pContext->Unmap(m_pCBPicking, 0);
}
