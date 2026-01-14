// ==========================================================================================
// 
// File Name: texture.h
// Date: 2025/08/16
// Author: Gu Anyi
// Description: Texture class header file
// 
// ==========================================================================================
#ifndef TEXTURE_H
#define TEXTURE_H

#include <wrl/client.h>
#include <d3d11.h>

class Texture
{
private:

	Microsoft::WRL::ComPtr<ID3D11Resource> m_pTexture = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTextureView = nullptr;
	unsigned int m_width;
	unsigned int m_height;

	//void Release();

public:

	Texture() : m_width(0), m_height(0) {}
	~Texture();

	bool IsLoaded() const
	{
		return m_pTextureView != nullptr;
	}

	void Release();
	bool Load(const wchar_t* pFilename);

	void SetTexture(int slot = 0) const;

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

	ID3D11ShaderResourceView* GetSRV() const { return m_pTextureView.Get(); }
};

#endif //TEXTURE_H