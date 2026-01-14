/*==============================================================================

   各ピクセルにobjectIdを書き込む用shader [picking_shader.h]
														 Author : Gu Anyi
														 Date   : 2026/01/05
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PICKING_SHADER_H
#define PICKING_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

class PickingShader
{
public:

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Finalize();

	void Begin();

	void SetParams(
		const DirectX::XMMATRIX& world,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& proj,
		uint32_t objectId
	);
	

private:

	// No need to release
	ID3D11Device*        m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// d3d11 resources
	ID3D11VertexShader* m_pVertexShader = nullptr;
	ID3D11PixelShader* m_pPixelShader = nullptr;
	ID3D11InputLayout* m_pInputLayout = nullptr;

	// constant buffer
	ID3D11Buffer* m_pCBPicking = nullptr;

private:

	struct CBPicking
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 proj;
		uint32_t objectId;
		uint32_t pad[3];
	};

};


#endif // PICKING_SHADER_H


/*==============================================================================
	Picking Shader（選択判定用シェーダー）

	目的：
		ピクセルが「どのオブジェクトに属しているか」を
		objectIdとしてGPUに書き込むための最小シェーダー

	Vertex Shader：
		- 頂点座標をmodel(world) -> view -> projectionの順で変換し、SV_POSITIONを出力する
		- 法線、UV、ライト等は一切行わない

	Pixel Shader：
		- objectId（uint）だけをRenderTargetに書き込む

	Constant Buffer：
		- world行列
		- view行列
		- projection行列
		- objectId

	RenderTarget：
		- フォーマット：R32_UINT
		- 1ピクセル = 1 objectId

	このシェーダーの役割：
		- 「このピクセルは誰のものか」をGPUに記録するだけ
==============================================================================*/
