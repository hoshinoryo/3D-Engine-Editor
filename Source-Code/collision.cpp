/*==============================================================================

Å@ 3DÉRÉäÉWÉáÉìîªíË [collision.cpp]
                                                         Author : Youhei Sato
                                                         Date   : 2025/12/23
--------------------------------------------------------------------------------

==============================================================================*/

#include "collision.h"
#include "texture.h"
#include "direct3d.h"
#include "line_shader.h"
#include "debug_ostream.h"
#include "draw3d.h"
#include "model_asset.h"
#include "model_renderer.h"
#include "mesh_object.h"
#include "axis_util.h"

#include <algorithm>

using namespace DirectX;

static constexpr int NUM_VERTEX = 5000;
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static Texture g_WhiteTexId;
static LineShader g_LineCollisionShader;

std::vector<AABB> g_Colliders;


struct VertexCollision
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};


bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&b.center);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
}

bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&point);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac);

	return a.radius * a.radius > XMVectorGetX(lsq);
}

bool Collision_IsOverlapCircle(const Circle& a, const Circle& b)
{

	float x1 = b.center.x - a.center.x;
	float y1 = b.center.y - a.center.y;

	return (a.radius + b.radius) * (a.radius + b.radius) > (x1 * x1 + y1 * y1);
}

bool Collision_IsOverlapBox(const Box& a, const Box& b)
{
	float at = a.center.y - a.half_height;
	float ab = a.center.y + a.half_height;
	float al = a.center.x - a.half_width;
	float ar = a.center.x + a.half_width;
	float bt = b.center.y - b.half_height;
	float bb = b.center.y + b.half_height;
	float bl = b.center.x - b.half_width;
	float br = b.center.x + b.half_width;
	return al < br && ar > bl && at < bb && ab > bt;

}

bool Collision_IsOverlapAABB(const AABB& a, const AABB& b)
{
	return a.min.x < b.max.x
		&& a.max.x > b.min.x
		&& a.min.y < b.max.y
		&& a.max.y > b.min.y
		&& a.min.z < b.max.z
		&& a.max.z > b.min.z;
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit{};

	hit.isHit = Collision_IsOverlapAABB(a, b);

	if (!hit.isHit) {
		return hit;
	}

	float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	// choose shallow axis
	bool isShallowX = false;
	bool isShallowY = false;
	bool isShallowZ = false;

	if (xdepth > ydepth) 
	{
		if (ydepth > zdepth) { isShallowZ = true; }
		else { isShallowY = true; }
	}
	else
	{
		if (zdepth > xdepth) { isShallowX = true; }
		else { isShallowZ = true; }
	}

	// axis normal direction : from a -> b
	XMFLOAT3 a_center = a.GetCenter();
	XMFLOAT3 b_center = b.GetCenter();
	XMVECTOR normal = XMLoadFloat3(&b_center) - XMLoadFloat3(&a_center);

	if (isShallowX) 
	{
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f,0.0f,0.0f });
		hit.depth = xdepth;
	}
	else if (isShallowY)
	{
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f,1.0f,0.0f });
		hit.depth = ydepth;
	}
	else if (isShallowZ)
	{
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f,0.0f,1.0f });
		hit.depth = zdepth;
	}

	XMStoreFloat3(&hit.normal, normal);

	// safety : if normal becomes invalid, fall back upward
	const float lenSq = 
		hit.normal.x * hit.normal.x +
		hit.normal.y * hit.normal.y +
		hit.normal.z * hit.normal.z;
	if (lenSq < 1e-8f)
	{
		hit.normal = { 0.0f, 1.0f, 0.0f }; // depth stays as computed
	}

	return hit;
}

// Converts a local-space AABB to world space
AABB Collision_TransformAABB(const AABB& aabb, const XMMATRIX& world)
{
	const XMFLOAT3& min = aabb.min;
	const XMFLOAT3& max = aabb.max;

	XMVECTOR corners[8] =
	{
		XMVectorSet(min.x, min.y, min.z, 1),
		XMVectorSet(max.x, min.y, min.z, 1),
		XMVectorSet(min.x, max.y, min.z, 1),
		XMVectorSet(max.x, max.y, min.z, 1),
		XMVectorSet(min.x, min.y, max.z, 1),
		XMVectorSet(max.x, min.y, max.z, 1),
		XMVectorSet(min.x, max.y, max.z, 1),
		XMVectorSet(max.x, max.y, max.z, 1),
	};

	XMFLOAT3 outMin{ FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 outMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (int i = 0; i < 8; i++)
	{
		XMVECTOR p = XMVector3TransformCoord(corners[i], world);
		XMFLOAT3 pf;
		XMStoreFloat3(&pf, p);

		outMin.x = std::min(outMin.x, pf.x);
		outMin.y = std::min(outMin.y, pf.y);
		outMin.z = std::min(outMin.z, pf.z);

		outMax.x = std::max(outMax.x, pf.x);
		outMax.y = std::max(outMax.y, pf.y);
		outMax.z = std::max(outMax.z, pf.z);
	}

	return { outMin, outMax };
}

// Moves a world-space AABB by a translation vector
AABB Collision_TranslateAABB(const AABB& aabb, const DirectX::XMFLOAT3& delta)
{
	AABB out = aabb;
	out.min.x += delta.x;
	out.min.y += delta.y;
	out.min.z += delta.z;
	out.max.x += delta.x;
	out.max.y += delta.y;
	out.max.z += delta.z;

	return out;
}


void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;


	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VertexCollision) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexId.Load(L"resource/white.png");
}

void Collision_DebugFinalize()
{
	SAFE_RELEASE(g_pVertexBuffer)
}

void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4 color)
{
	int numVertex = (int)(circle.radius * 2.0f * XM_PI + 1);

	//Shader_Begin();
	g_LineCollisionShader.Begin();

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	VertexCollision* v = (VertexCollision*)msr.pData;

	const float rad = XM_2PI / numVertex;

	for (int i = 0; i < numVertex; i++) {
		v[i].position.x = cosf(rad * i) * circle.radius + circle.center.x;
		v[i].position.y = sinf(rad * i) * circle.radius + circle.center.y;
		v[i].position.z = 0.0f;
		v[i].color = color;
		//v[i].texcoord = { 0.0f,0.0f };
	}

	g_pContext->Unmap(g_pVertexBuffer, 0);

	g_LineCollisionShader.SetWorldMatrix(XMMatrixIdentity());

	UINT stride = sizeof(VertexCollision);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	g_WhiteTexId.SetTexture(0);

	g_pContext->Draw(numVertex, 0);
}

void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4 color)
{
	g_LineCollisionShader.Begin();

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	VertexCollision* v = (VertexCollision*)msr.pData;

	v[0].position = { box.center.x - box.half_width, box.center.y - box.half_height, 0.0f };
	v[1].position = { box.center.x + box.half_width, box.center.y - box.half_height, 0.0f };
	v[2].position = { box.center.x + box.half_width, box.center.y + box.half_height, 0.0f };
	v[3].position = { box.center.x - box.half_width, box.center.y + box.half_height, 0.0f };
	v[4].position = { box.center.x - box.half_width, box.center.y - box.half_height, 0.0f };

	for (int i = 0; i < 5; i++) {
		v[i].color = color;
	}

	g_pContext->Unmap(g_pVertexBuffer, 0);

	g_LineCollisionShader.SetWorldMatrix(XMMatrixIdentity());

	UINT stride = sizeof(VertexCollision);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	g_WhiteTexId.SetTexture(0);

	g_pContext->Draw(5, 0);
}

void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4 color)
{
	Draw3d_MakeWireBox(aabb.GetCenter(), aabb.GetHalf(), color);
}


std::vector<AABB> CollisionSystem::AllColliders()
{
	return g_Colliders;
}

void CollisionSystem::ClearColliders()
{
	g_Colliders.clear();
}

void CollisionSystem::AddCollidersAABB(const AABB& aabb)
{
	g_Colliders.push_back(aabb);
}

// Resolves collisions between the player's AABB and static AABBs
// by iteratively pushing the player out along the minimum penetration axis
bool CollisionSystem::ResolveAgainstScene(
	const IAABBProvider& playerAABBProvider,
	XMFLOAT3& position,
	int maxIterations,
	bool* outGround
)
{
	if (outGround) *outGround = false;

	bool anyHit = false;
	AABB playerAABB = playerAABBProvider.GetAABB();

	for (int iter = 0; iter < maxIterations; iter++)
	{
		bool hitThisIter = false;

		for (const AABB& s : g_Colliders)
		{
			//if (!obj.visible) continue;
			//if (!obj.aabbValid) continue;

			//const AABB& s = obj.worldAABB;

			Hit hit = Collision_IsHitAABB(playerAABB, s);
			if (!hit.isHit) continue;

			anyHit = true;
			hitThisIter = true;

			XMFLOAT3 n = hit.normal; // âüÇµñﬂÇµï˚å¸

			const float lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
			if (lenSq < 1e-8f) n = { 0.0f, 1.0f, 0.0f };

			float depth = hit.depth;
			const float epsilon = 0.0005f;
			depth += epsilon;

			XMFLOAT3 delta = { -n.x * depth, -n.y * depth, -n.z * depth };

			position.x += delta.x;
			position.y += delta.y;
			position.z += delta.z;

			playerAABB = Collision_TranslateAABB(playerAABB, delta);

			if (outGround)
			{
				const float ny = fabsf(n.y);
				const float nx = fabsf(n.x);
				const float nz = fabsf(n.z);

				if (ny >= nx && ny >= nz && delta.y > 0.0f) *outGround = true;
			}
		}

		if (!hitThisIter) break;
	}

	return anyHit;
}
