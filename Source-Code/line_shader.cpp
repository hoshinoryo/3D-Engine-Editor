/*==============================================================================

   Line shader [line_shader.cpp]
														 Author : Gu Anyi
														 Date   : 2025/12/17
--------------------------------------------------------------------------------

==============================================================================*/

#include <fstream>

#include "line_shader.h"
#include "direct3d.h"
#include "debug_ostream.h"

using namespace DirectX;

bool LineShader::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr;

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) return false;
	m_pDevice = pDevice;
	m_pContext = pContext;

	// コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("shader_vertex_line.cso", std::ios::binary);
	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_line.cso", "エラー", MB_OK);
		return false;
	}
	ifs_vs.seekg(0, std::ios::end);
	std::streamsize filesize = ifs_vs.tellg();
	ifs_vs.seekg(0, std::ios::beg);
	unsigned char* vsbinary_pointer = new unsigned char[filesize];
	ifs_vs.read((char*)vsbinary_pointer, filesize);
	ifs_vs.close();

	// ---- Shader program ----
	// 頂点シェーダーの作成
	hr = m_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &m_pVertexShader);
	if (FAILED(hr)) {
		hal::dout << "頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer;
		return false;
	}

	// 頂点レイアウト
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout);
	hr = m_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &m_pInputLayout);
	delete[] vsbinary_pointer;
	if (FAILED(hr)) {
		hal::dout << "頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}

	// WorldCBの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;

	hr = m_pDevice->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferWorld);
	if (FAILED(hr))
	{
		hal::dout << "WorldCBの作成に失敗しました" << std::endl;
		return false;
	}

	// コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_line.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_line.cso", "エラー", MB_OK);
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

	if (FAILED(hr)) {
		hal::dout << "ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	return true;
}

void LineShader::Finalize()
{
	SAFE_RELEASE(m_pVSConstantBufferWorld);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);
}

void LineShader::Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBufferWorld);
	//m_pContext->VSSetConstantBuffers(1, 1, &m_pVSConstantBufferView);
	//m_pContext->VSSetConstantBuffers(2, 1, &m_pVSConstantBufferProj);
}

void LineShader::SetWorldMatrix(const DirectX::XMMATRIX& mtxWorld)
{
	XMFLOAT4X4 transpose;

	XMStoreFloat4x4(&transpose, XMMatrixTranspose(mtxWorld));

	m_pContext->UpdateSubresource(m_pVSConstantBufferWorld, 0, nullptr, &transpose, 0, 0);
}

/*
void LineShader::SetViewProjBuffers(ID3D11Buffer* view, ID3D11Buffer* proj)
{
	m_pVSConstantBufferView = view;
	m_pVSConstantBufferProj = proj;
}
*/
