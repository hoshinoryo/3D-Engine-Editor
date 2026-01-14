/*==============================================================================

　 cubeの表示[cube.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/05
--------------------------------------------------------------------------------

==============================================================================*/


#include "cube.h"
#include "direct3d.h"
#include "default3Dshader.h"
#include "texture.h"

#include <DirectXMath.h>


using namespace DirectX;

static constexpr int NUM_VERTEX = 4 * 6; // 頂点数
static constexpr int NUM_INDEX  = 3 * 2 * 6; // インデックス数

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

static Texture g_CubeTex;

struct VertexCube
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;   // 法線
	XMFLOAT3 tangent;  // 切線
	XMFLOAT4 color;    // 色
	XMFLOAT2 texcoord; // UV
};


// vertexの結び方
// unsigned shortによって頂点数が多くとも65525番目まで
// high polygonのときunsigned intを使ってください
static unsigned short g_CubeIndex[]
{
	0,  1,  2,  0,  2,  3,
	4,  5,  6,  4,  6,  7,
	8,  9,  10, 8,  10, 11,
	12, 13, 14, 12, 14, 15,
	16, 17, 18, 16, 18, 19,
	20, 21, 22, 20, 22, 23
};


void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, float halfExtent)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	const float s = halfExtent;

	VertexCube g_CubeVertex[NUM_VERTEX] // 前半分座標、後ろ半分色
	{
		// Front (1)
		{ { -s,  s, -s }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } }, // 0
		{ {  s,  s, -s }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } }, // 1
		{ {  s, -s, -s }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } }, // 2
		{ { -s, -s, -s }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } }, // 3

		// Back (2)
		{ {  s,  s,  s }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } }, // 4
		{ { -s,  s,  s }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } }, // 5
		{ { -s, -s,  s }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } }, // 6
		{ {  s, -s,  s }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } }, // 7

		// Left (3)
		{ { -s,  s,  s }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ { -s,  s, -s }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ { -s, -s, -s }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ { -s, -s,  s }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },

		// Right (4)
		{ {  s,  s, -s }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ {  s,  s,  s }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ {  s, -s,  s }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ {  s, -s, -s }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },

		// Top (5)
		{ { -s,  s,  s }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ {  s,  s,  s }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ {  s,  s, -s }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ { -s,  s, -s }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },

		// Bottom (6)
		{ { -s, -s, -s }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ {  s, -s, -s }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ {  s, -s,  s }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ { -s, -s,  s }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } },
	};

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VertexCube) * NUM_VERTEX; // sizeof(g_CubeVertex)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_CubeVertex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX; // sizeof(g_CubeIndex)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
;
	sd.pSysMem = g_CubeIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

	g_CubeTex.Load(L"resources/backgroundCube_Texture.png");
}

void Cube_Finalize(void)
{
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void Cube_DrawMesh(const XMMATRIX& mtxWorld)
{
	g_Default3DshaderStatic.Begin();

	// ピクセルシェーダ―に色を設定
	g_Default3DshaderStatic.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// テクスチャ設定
	g_CubeTex.SetTexture();

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(VertexCube);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプランを設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	//Shader3d_SetWorldMatrix(mtxWorld); // 頂点シェーダ―にワールド座標変換行列を設定
	g_Default3DshaderStatic.SetWorldMatrix(mtxWorld);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ポリゴン描画命令発行
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

CubeObject::CubeObject(float halfExtent)
	: m_HalfExtent(halfExtent)
{
	m_Position = { 0, 0, 0 };
	m_Scale    = { 1, 1, 1 };
	UpdateAABB();
}

void CubeObject::SetPosition(const XMFLOAT3& pos)
{
	m_Position = pos;
	UpdateAABB();
}

void CubeObject::SetScale(const XMFLOAT3& scl)
{
	m_Scale = scl;
	UpdateAABB();
}

void CubeObject::UpdateAABB()
{
	const XMFLOAT3 half = {
		m_HalfExtent * m_Scale.x,
		m_HalfExtent * m_Scale.y,
		m_HalfExtent * m_Scale.z,
	};

	m_AABB.min = { m_Position.x - half.x, m_Position.y - half.y, m_Position.z - half.z };
	m_AABB.max = { m_Position.x + half.x, m_Position.y + half.y, m_Position.z + half.z };
}

void CubeObject::Draw() const
{
	const XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
		* XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	Cube_DrawMesh(world);
}
