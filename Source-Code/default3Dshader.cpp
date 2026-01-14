/*==============================================================================

   Default 3d shader program [default3Dshader.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/

#include "default3Dshader.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <fstream>

#include "direct3d.h"
#include "debug_ostream.h"


using namespace DirectX;

Default3DShader g_Default3DshaderStatic;
Default3DShader g_Default3DshaderSkinned;

struct SpecularData
{
	XMFLOAT3 CameraPosition;
	float Power;
	XMFLOAT4 Color;
};


bool Default3DShader::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, Variant variant)
{
	HRESULT hr;

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) return false;
	m_pDevice = pDevice;
	m_pContext = pContext;

	// コンパイル済み頂点シェーダーの読み込み
	//std::ifstream ifs_vs("shader_vertex_3d.cso", std::ios::binary);
	const char* vsFile =
		(variant == Variant::Skinned) ? "shader_vertex_3d_skinned.cso"
		                              : "shader_vertex_3d_static.cso";
	std::ifstream ifs_vs(vsFile, std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_3d.cso", "エラー", MB_OK);
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
	if (variant == Variant::Static)
	{
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsbinary_pointer, filesize, &m_pInputLayout);
	}
	else
	{
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsbinary_pointer, filesize, &m_pInputLayout);
	}
	
	delete[] vsbinary_pointer;

	if (FAILED(hr)) {
		hal::dout << "頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	m_pDevice->CreateBuffer(&buffer_desc, nullptr, &m_pVSConstantBufferWorld);

	// コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("shader_pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_3d.cso", "エラー", MB_OK);
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

	// ピクセルシェーダー用定数バッファの作成
	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	m_pDevice->CreateBuffer(&buffer_desc, nullptr, &m_pPSConstantBuffer0);

	buffer_desc.ByteWidth = sizeof(SpecularData); // Specular pixel constant buffer
	m_pDevice->CreateBuffer(&buffer_desc, nullptr, &m_pPSConstantBuffer3);

	return true;
}

void Default3DShader::Finalize()
{
	SAFE_RELEASE(m_pPSConstantBuffer3);
	SAFE_RELEASE(m_pPSConstantBuffer0);
	SAFE_RELEASE(m_pVSConstantBufferWorld);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexShader);
}

void Default3DShader::SetWorldMatrix(const XMMATRIX& matrix)
{
	XMFLOAT4X4 transpose;

	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	m_pContext->UpdateSubresource(m_pVSConstantBufferWorld, 0, nullptr, &transpose, 0, 0);
}

void Default3DShader::SetColor(const XMFLOAT4& color)
{
	m_pContext->UpdateSubresource(m_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}


// Specular part
void Default3DShader::UpdateSpecularParams(XMFLOAT3 cameraPos, float power, const XMFLOAT4& color)
{
	SpecularData data;

	data.CameraPosition = cameraPos;
	data.Power = power;
	data.Color = color;

	m_pContext->UpdateSubresource(m_pPSConstantBuffer3, 0, nullptr, &data, 0, 0);
}

void Default3DShader::Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	m_pContext->IASetInputLayout(m_pInputLayout);

	// 定数バッファを描画パイプラインに設定
	m_pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBufferWorld);

	m_pContext->PSSetConstantBuffers(0, 1, &m_pPSConstantBuffer0);
	m_pContext->PSSetConstantBuffers(3, 1, &m_pPSConstantBuffer3);
}

