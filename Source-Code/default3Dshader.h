/*==============================================================================

   Default 3d shader program [default3Dshader.h]
														 Author : Gu Anyi
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEFAULT_3D_SHADER_H
#define	DEFAULT_3D_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>


class Default3DShader
{
private:

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// d3d11 resources
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;

	// VS constant buffer
	ID3D11Buffer* m_pVSConstantBufferWorld = nullptr; // matrix for local to world(b0)

	// PS constant buffer
	ID3D11Buffer* m_pPSConstantBuffer0 = nullptr; // diffuse color for pixel shader
	ID3D11Buffer* m_pPSConstantBuffer3 = nullptr; // specular color for pixel shader

public:

	enum class Variant
	{
		Static,
		Skinned
	};

	Default3DShader() = default;
	~Default3DShader() = default;

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, Variant variant);
	void Finalize();

	void Begin();

	void SetWorldMatrix(const DirectX::XMMATRIX& matrix);

	void SetColor(const DirectX::XMFLOAT4& color); // diffuse color

	void UpdateSpecularParams(DirectX::XMFLOAT3 cameraPos, float power, const DirectX::XMFLOAT4& color);

	ID3D11VertexShader* GetVertexShader() const { return m_pVertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return m_pPixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }

	//void DebugDraw(DirectX::XMFLOAT3 cameraPos);
};

extern Default3DShader g_Default3DshaderStatic;
extern Default3DShader g_Default3DshaderSkinned;

#endif // DEFAULT_3D_SHADER_H
