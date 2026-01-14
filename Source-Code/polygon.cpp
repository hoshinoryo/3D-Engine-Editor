/*==============================================================================

   ポリゴン描画 [polygon.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"

using namespace DirectX;

#pragma comment(lib, "d3d11.lib") // ライブラリリンク

// static constexpr int NUM_VERTEX = 4; // 頂点数

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11ShaderResourceView* g_pTexture = nullptr; // テクスチャ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 円を描画のために
static int g_NumVertex = 0;
static float g_Radius = 100.0f;
static float g_Cx = 1000.0f; // 円の半径
static float g_Cy = 500.0f;  // 円の中心座標

// 頂点構造体
struct Vertex
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;    // 色
	XMFLOAT2 uv;       // UV
};


void Polygon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Polygon_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 点の数を算出
	g_NumVertex = (int)g_Radius * 2.0f * XM_PI;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * g_NumVertex;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	// テクスチャの読み込み
	TexMetadata metadata;
	ScratchImage image;

	LoadFromWICFile(L"resources/white.png", WIC_FLAGS_NONE, &metadata, image);
	HRESULT hr = CreateShaderResourceView(g_pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_pTexture);

	if (FAILED(hr))
	{
		MessageBox(nullptr, "テクスチャの読み込みに失敗しました", "エラー", MB_OK | MB_ICONERROR);
	}
}

void Polygon_Finalize(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexBuffer);
}

void Polygon_Draw(void)
{
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	// 頂点情報を書き込み
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	// 円の描画
	const float rad = XM_2PI / g_NumVertex; // 円周を頂点数で等分したときの、1つあたりのラジアン角を計算
	for (int i = 0; i < g_NumVertex; i++)
	{
		v[i].position.x = cosf(rad * i) * g_Radius + g_Cx;
		v[i].position.y = sinf(rad * i) * g_Radius + g_Cy;
		v[i].position.z = 0.0f;
		v[i].color = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
		v[i].uv = XMFLOAT2{ 0.5, 0.5f }; // UV座標の仮設定（中央に固定）
	}

	/*
	v[0].position = {x,     y,     0.0f};
	v[1].position = { x + w, y,     0.0f };
	v[2].position = { x ,    y + h, 0.0f };
	v[3].position = { x + w, y + h, 0.0f };
	
	v[0].color = { 1.0f, 1.0f, 1.0f, 0.0f };
	v[1].color = { 1.0f, 1.0f, 1.0f, 0.0f };
	v[2].color = { 1.0f, 1.0f, 1.0f, 0.0f };
	v[3].color = { 1.0f, 1.0f, 1.0f, 0.0f };

	// 画面の左上uv値は(0, 0)、右下は(1, 1)
	v[0].uv = { 0.0f, 0.0f };
	v[1].uv = { 1.0f, 0.0f };
	v[2].uv = { 0.0f, 1.0f };
	v[3].uv = { 1.0f, 1.0f };
	*/

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// ワールド変換行列を設定
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// 頂点シェーダーに変換行列を設定
	Shader_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// テクスチャ設定
	// g_pContext->PSSetShaderResources(0, 1, &g_pTexture);

	// ポリゴン描画命令発行
	g_pContext->Draw(g_NumVertex, 0);
}
