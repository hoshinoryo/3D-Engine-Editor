/*==============================================================================

   Debug drawing [draw3d.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#include "draw3d.h"
#include "direct3d.h"
#include "line_shader.h"

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

struct VertexLine
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;    // 色
};

static std::vector<VertexLine> g_vertices;
static size_t g_vertexCapacity = 0;

static LineShader g_LineShader;

static VertexLine MakeVertex(const XMFLOAT3& pos, const XMFLOAT4& color);
static void EnsureVertexBuffer(size_t vertexCount);


void Draw3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_LineShader.Initialize(pDevice, pContext);
}

void Draw3d_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
	g_vertices.clear();

	g_LineShader.Finalize();
}

void Draw3d_Draw(void)
{
	if (!g_pDevice || !g_pContext) return;
	if (g_vertices.empty()) return;

	EnsureVertexBuffer(g_vertices.size());
	if (!g_pVertexBuffer) return;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData,
			g_vertices.data(),
			sizeof(VertexLine) * g_vertices.size());
		g_pContext->Unmap(g_pVertexBuffer, 0);
	}
	else
	{
		g_vertices.clear();
		return;
	}

	// Default shader binding
	g_LineShader.Begin();

	XMMATRIX mtxWorld = XMMatrixIdentity();
	g_LineShader.SetWorldMatrix(mtxWorld);

	UINT stride = sizeof(VertexLine);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	g_pContext->Draw(static_cast<UINT>(g_vertices.size()), 0);

	g_vertices.clear();
}

void Draw3d_MakeLine(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT4& color)
{
	g_vertices.push_back(MakeVertex(p0, color));
	g_vertices.push_back(MakeVertex(p1, color));
}

void Draw3d_MakeCross(const DirectX::XMFLOAT3& center, float size, const DirectX::XMFLOAT4& color)
{
	XMFLOAT3 px1(center.x - size, center.y,        center.z);
	XMFLOAT3 px2(center.x + size, center.y,        center.z);
	XMFLOAT3 py1(center.x,        center.y - size, center.z);
	XMFLOAT3 py2(center.x,        center.y + size, center.z);
	XMFLOAT3 pz1(center.x,        center.y,        center.z - size);
	XMFLOAT3 pz2(center.x,        center.y,        center.z + size);

	Draw3d_MakeLine(px1, px2, color);
	Draw3d_MakeLine(py1, py2, color);
	Draw3d_MakeLine(pz1, pz2, color);
}

void Draw3d_MakeWireBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& halfSize, const DirectX::XMFLOAT4& color)
{
	// 8 corners
	XMFLOAT3 c[8];
	c[0] = XMFLOAT3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z);
	c[1] = XMFLOAT3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z);
	c[2] = XMFLOAT3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z);
	c[3] = XMFLOAT3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z);
	c[4] = XMFLOAT3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z);
	c[5] = XMFLOAT3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z);
	c[6] = XMFLOAT3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z);
	c[7] = XMFLOAT3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z);

	// 12 edges
	Draw3d_MakeLine(c[0], c[1], color);
	Draw3d_MakeLine(c[1], c[2], color);
	Draw3d_MakeLine(c[2], c[3], color);
	Draw3d_MakeLine(c[3], c[0], color);

	Draw3d_MakeLine(c[4], c[5], color);
	Draw3d_MakeLine(c[5], c[6], color);
	Draw3d_MakeLine(c[6], c[7], color);
	Draw3d_MakeLine(c[7], c[4], color);

	Draw3d_MakeLine(c[0], c[4], color);
	Draw3d_MakeLine(c[1], c[5], color);
	Draw3d_MakeLine(c[2], c[6], color);
	Draw3d_MakeLine(c[3], c[7], color);
}

void Draw3d_MakeWireSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color, int segments)
{
	if (segments < 3) segments = 3;

	const float step = XM_2PI / segments;

	// ---- XY plane ----
	for (int i = 0; i < segments; i++)
	{
		float a0 = i * step;
		float a1 = (i + 1) * step;
		
		XMFLOAT3 p0(
			center.x + radius * cos(a0),
			center.y + radius * sin(a0),
			center.z
		);
		XMFLOAT3 p1(
			center.x + radius * cos(a1),
			center.y + radius * sin(a1),
			center.z
		);

		Draw3d_MakeLine(p0, p1, color);
	}

	// ---- XZ plane ----
	for (int i = 0; i < segments; i++)
	{
		float a0 = i * step;
		float a1 = (i + 1) * step;

		XMFLOAT3 p0(
			center.x + radius * cos(a0),
			center.y,
			center.z + radius * sin(a0)
		);
		XMFLOAT3 p1(
			center.x + radius * cos(a1),
			center.y,
			center.z + radius * sin(a1)
		);

		Draw3d_MakeLine(p0, p1, color);
	}

	// ---- YZ plane ----
	for (int i = 0; i < segments; i++)
	{
		float a0 = i * step;
		float a1 = (i + 1) * step;

		XMFLOAT3 p0(
			center.x,
			center.y + radius * cos(a0),
			center.z + radius * sin(a0)
		);
		XMFLOAT3 p1(
			center.x,
			center.y + radius * cos(a1),
			center.z + radius * sin(a1)
		);

		Draw3d_MakeLine(p0, p1, color);
	}
}

static VertexLine MakeVertex(const XMFLOAT3& pos, const XMFLOAT4& color)
{
	VertexLine v{};

	v.position = pos;
	//v.tangent  = XMFLOAT3(1.0f, 0.0f, 0.0f);
	//v.normal   = XMFLOAT3(0.0f, 1.0f, 0.0f);
	v.color    = color;
	//v.texcoord = XMFLOAT2(0.0f, 0.0f);

	return v;
}

static void EnsureVertexBuffer(size_t vertexCount)
{
	if (!g_pDevice) return;
	
	// すでにバッファがあり、容量も足りている → そのまま使う
	if (g_pVertexBuffer && g_vertexCapacity >= vertexCount)
		return;
	
	// それ以外の場合 → 作り直す
	SAFE_RELEASE(g_pVertexBuffer);

	g_vertexCapacity = vertexCount > 0 ? vertexCount : 2;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(VertexLine) * g_vertexCapacity);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = g_pDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);
	if (FAILED(hr))
	{
		g_pVertexBuffer = nullptr;
		g_vertexCapacity = 0;
	}
}
