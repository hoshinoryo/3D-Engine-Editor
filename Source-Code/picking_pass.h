/*==============================================================================

   Offscreen rendering and object id management [picking_pass.h]
														 Author : Gu Anyi
														 Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PICKING_PASS_H
#define PICKING_PASS_H

#include <d3d11.h>
#include <DirectXMath.h>

#include "picking_shader.h"
#include "d3d11_state_guard_util.h"

struct ModelAsset;

class PickingPass
{
public:

	bool Initialize(uint32_t width, uint32_t height);
	void Finalize();

	bool Resize(uint32_t width, uint32_t height);

	// Pick and begin to render
	void Begin(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
	void End();

	//void DrawAsset(ModelAsset* asset, const DirectX::XMMATRIX& world, uint32_t objectId);
	void DrawAsset(ModelAsset* asset, uint32_t meshIndex, const DirectX::XMMATRIX& world, uint32_t objectId);

	uint32_t ReadBackId(int mouseX, int mouseY);

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	ID3D11ShaderResourceView* GetIdSRV() { return m_IdSRV; }

private:

	bool CreateTargets(uint32_t width, uint32_t height);
	void ReleaseTargets();

	bool CreateReadback();
	void ReleaseReadback();

	bool CreateFixedStates();
	void ReleaseFixedStates();

private:

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	ID3D11Device*        m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// Offscreen RT: R32_UINT
	ID3D11Texture2D*        m_IdTex = nullptr;
	ID3D11RenderTargetView* m_IdRTV = nullptr;

	// Depth: D24S8
	ID3D11Texture2D*        m_DepthTex = nullptr;
	ID3D11DepthStencilView* m_DepthDSV = nullptr;

	// Readback staging (1x1) for pixel
	ID3D11Texture2D* m_ReadBack1x1 = nullptr;

	ID3D11ShaderResourceView* m_IdSRV = nullptr;

	PickingShader m_PickingShader;

	// View/Proj
	DirectX::XMMATRIX m_View = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_Proj = DirectX::XMMatrixIdentity();

	// Viewpoint for picking RT
	D3D11_VIEWPORT m_VP{};

	// Fixed states (create once)
	ID3D11BlendState*        m_BS_NoBlend_WriteAll = nullptr;
	ID3D11RasterizerState*   m_RS_NoCull_NoScissor = nullptr;
	ID3D11DepthStencilState* m_DSS_DepthLessEqual_NoStencil = nullptr;
	
	//Save and restore states
	D3D11StateGuard m_StateGuard;
};

#endif // PICKING_PASS_H



/*==============================================================================
	Picking Pass（オフスクリーン描画）

	目的：
		画面上の「このピクセルは、どのオブジェクトのものか」を
		GPU に判断させるための専用レンダリングパス

	何をしているか：
		1. 通常描画とは別に、シーンをもう一度描画する
		2. 画面には表示せず、オフスクリーンのRenderTargetに描画する
		3. 各オブジェクトは「objectId」を出力する
		4. 深度テストを使い、手前のオブジェクトだけが残る
		5. マウス座標のピクセルをCPUに読み戻す（ReadBack）

	RenderTarget：
		- フォーマット：R32_UINT
		- 各ピクセルには1つのobjectId（整数）だけが入る

	クラスの役割：
		- RenderTarget/DepthBufferの作成と管理
		- Begin/Endによる描画パスの制御
		- ピクセルのobjectIdをCPU側へ読み戻す

	注意：
		描画内容（objectIdの出力はPickingShaderに任せる
==============================================================================*/