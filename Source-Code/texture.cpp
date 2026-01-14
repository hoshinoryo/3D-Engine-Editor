// ==========================================================================================
// 
// File Name: texture.cpp
// Date: 2025/09/26
// Author: Gu Anyi
// Description: Manage texture initialization and release
// 
// ==========================================================================================

#include "texture.h"
#include "direct3d.h"
#include "WICTextureLoader11.h"

#include <string>
#include <iostream>

using namespace DirectX;

extern ID3D11Device* Direct3D_GetDevice();
extern ID3D11DeviceContext* Direct3D_GetContext();


Texture::~Texture()
{
	Release();
}

void Texture::Release()
{
	m_pTexture.Reset();
	m_pTextureView.Reset();
	m_width = 0;
	m_height = 0;
}

bool Texture::Load(const wchar_t* pFilename)
{
	Release();

	ID3D11Device* g_pDevice = Direct3D_GetDevice();
	if (!g_pDevice)
	{
		std::cerr << "Error: Direct3D device is not initialized." << std::endl;
		return false;
	}

	HRESULT hr = CreateWICTextureFromFile(
		g_pDevice,
		pFilename,
		m_pTexture.GetAddressOf(),
		m_pTextureView.GetAddressOf()
	);
	if (FAILED(hr))
	{
		std::cerr << "Error: Failed to load texture from WIC file" << std::endl;
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTex2D;
	if (SUCCEEDED(m_pTexture.As(&pTex2D)))
	{
		D3D11_TEXTURE2D_DESC t2desc;
		pTex2D->GetDesc(&t2desc);
		m_width = t2desc.Width;
		m_height = t2desc.Height;
		return true;
	}
	else
	{
		m_pTexture.Reset();
		m_pTextureView.Reset();
		std::cerr << "Error: Could not retrieve ID3D11Texture2D interface." << std::endl;
		return false;
	}
}

void Texture::SetTexture(int slot) const
{
	ID3D11DeviceContext* g_pContext = Direct3D_GetContext();
	if (!g_pContext)
	{
		std::cerr << "Error: Direct3D device context is not initialized." << std::endl;
		return;
	}

	ID3D11ShaderResourceView* tex = m_pTextureView.Get();
	g_pContext->PSSetShaderResources(slot, 1, &tex);
}

unsigned int Texture::GetWidth() const
{
	return m_width;
}

unsigned int Texture::GetHeight() const
{
	return m_height;
}
