/*==============================================================================

   Debug drawing [draw3d.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

#include "draw3d.h"
#include "direct3d.h"
#include "line_shader.h"
#include "camera_base.h"
#include "camera_manager.h"
#include "d3d11_state_guard_util.h"

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

struct VertexLine
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;    // 色
};

enum class Draw3dPrimitive
{
	Line,
	Triangle,
};

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11Buffer* g_pLineVB = nullptr;
static ID3D11Buffer* g_pTriVB = nullptr;
static std::vector<VertexLine> g_lineVertices;
static std::vector<VertexLine> g_triangleVertices;
static size_t g_lineVBCapacity = 0;
static size_t g_triVBCapacity = 0;

static LineShader g_LineShader;

static VertexLine MakeVertex(const XMFLOAT3& pos, const XMFLOAT4& color);
static void EnsureVertexBuffer(ID3D11Buffer*& vb, size_t vertexCount, size_t& capacity);

static void DrawVertexBatch(std::vector<VertexLine>& vertices, ID3D11Buffer*& vb, size_t& capacity, D3D11_PRIMITIVE_TOPOLOGY topology);


void Draw3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_LineShader.Initialize(pDevice, pContext);
}

void Draw3d_Finalize(void)
{
	SAFE_RELEASE(g_pLineVB);
	SAFE_RELEASE(g_pTriVB);
	g_lineVBCapacity = 0;
	g_triVBCapacity = 0;
	g_lineVertices.clear();
	g_triangleVertices.clear();

	g_LineShader.Finalize();
}

void Draw3d_Draw(void)
{
	if (!g_pDevice || !g_pContext) return;

	D3D11StateGuard guard(
		g_pContext,
		D3D11StateGuard::IABuffers |
		D3D11StateGuard::Topology |
		D3D11StateGuard::InputLayout |
		D3D11StateGuard::Shaders |
		D3D11StateGuard::Rasterizer |
		D3D11StateGuard::Viewports |
		D3D11StateGuard::Scissors |
		D3D11StateGuard::BlendStates |
		D3D11StateGuard::DepthStencil
	);

	g_LineShader.Begin();

	XMMATRIX mtxWorld = XMMatrixIdentity();
	g_LineShader.SetWorldMatrix(mtxWorld);
	
	// draw line
	DrawVertexBatch(
		g_lineVertices,
		g_pLineVB,
		g_lineVBCapacity,
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST
	);

	// draw triangle
	DrawVertexBatch(
		g_triangleVertices,
		g_pTriVB,
		g_triVBCapacity,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
}

void Draw3d_MakeLine(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT4& color)
{
	g_lineVertices.push_back(MakeVertex(p0, color));
	g_lineVertices.push_back(MakeVertex(p1, color));
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

void Draw3d_MakeThickLine(const XMFLOAT3& p0, const XMFLOAT3& p1, float thickness, const XMFLOAT4& color)
{
	CameraBase& cam = CameraManager::GetActiveCamera();

	XMVECTOR v0 = XMLoadFloat3(&p0);
	XMVECTOR v1 = XMLoadFloat3(&p1);

	XMVECTOR dir = XMVector3Normalize(v1 - v0);

	// camera view direction
	XMFLOAT3 f = cam.GetFront();
	XMVECTOR viewDir = XMVector3Normalize(XMLoadFloat3(&f));

	// perpendicular vector
	XMVECTOR side = XMVector3Cross(viewDir, dir);
	if (XMVectorGetX(XMVector3LengthSq(side)) < 1e-6f)
	{
		side = XMVector3Cross(XMVectorSet(0, 1, 0, 0), dir);
	}

	side = XMVector3Normalize(side) * (thickness * 0.5f);

	XMVECTOR a = v0 + side;
	XMVECTOR b = v1 + side;
	XMVECTOR c = v1 - side;
	XMVECTOR d = v0 - side;

	XMFLOAT3 fa, fb, fc, fd;
	XMStoreFloat3(&fa, a);
	XMStoreFloat3(&fb, b);
	XMStoreFloat3(&fc, c);
	XMStoreFloat3(&fd, d);

	// two triangles
	g_triangleVertices.push_back(MakeVertex(fa, color));
	g_triangleVertices.push_back(MakeVertex(fb, color));
	g_triangleVertices.push_back(MakeVertex(fc, color));

	g_triangleVertices.push_back(MakeVertex(fa, color));
	g_triangleVertices.push_back(MakeVertex(fc, color));
	g_triangleVertices.push_back(MakeVertex(fd, color));
}

static VertexLine MakeVertex(const XMFLOAT3& pos, const XMFLOAT4& color)
{
	VertexLine v{};

	v.position = pos;
	v.color    = color;

	return v;
}

static void EnsureVertexBuffer(ID3D11Buffer*& vb, size_t vertexCount, size_t& capacity)
{
	if (!g_pDevice) return;
	
	// すでにバッファがあり、容量も足りている → そのまま使う
	if (vb && capacity >= vertexCount) return;
	
	// それ以外の場合 → 作り直す
	SAFE_RELEASE(vb);

	capacity = vertexCount > 0 ? vertexCount : 2;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(VertexLine) * capacity);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = g_pDevice->CreateBuffer(&bd, nullptr, &vb);
	if (FAILED(hr))
	{
		vb = nullptr;
		capacity = 0;
	}
}

static void DrawVertexBatch(
	std::vector<VertexLine>& vertices,
	ID3D11Buffer*& vb,
	size_t& capacity,
	D3D11_PRIMITIVE_TOPOLOGY topology
)
{
	if (vertices.empty()) return;

	EnsureVertexBuffer(vb, vertices.size(), capacity);
	if (!vb)
	{
		vertices.clear();
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = g_pContext->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(hr))
	{
		vertices.clear();
		return;
	}

	const size_t bytes = sizeof(VertexLine) * vertices.size();
	memcpy(mapped.pData, vertices.data(), bytes);
	g_pContext->Unmap(vb, 0);

	UINT stride = sizeof(VertexLine);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	g_pContext->IASetPrimitiveTopology(topology);

	g_pContext->Draw(static_cast<UINT>(vertices.size()), 0);

	vertices.clear();
}
