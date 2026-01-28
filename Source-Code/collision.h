/*==============================================================================

Å@ 3DÉRÉäÉWÉáÉìîªíË [collision.h]
														 Author : Youhei Sato
														 Date   : 2025/12/23
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef COLLISION_H
#define COLLISION_H

#include <d3d11.h>
#include <vector>
#include <DirectXMath.h>

#include "aabb_provider.h"

struct ModelAsset;
struct MeshObject;

struct Sphere
{
	DirectX::XMFLOAT3 center;
	float radius;
};

struct Circle
{
	DirectX::XMFLOAT2 center;
	float radius;
};

struct Box
{
	DirectX::XMFLOAT2 center;
	float half_width;
	float half_height;
};

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;

	DirectX::XMFLOAT3 GetHalf() const {
		DirectX::XMFLOAT3 half;
		half.x = (max.x - min.x) * 0.5f;
		half.y = (max.y - min.y) * 0.5f;
		half.z = (max.z - min.z) * 0.5f;
		return half;
	}

	DirectX::XMFLOAT3 GetCenter() const {
		DirectX::XMFLOAT3 center;
		DirectX::XMFLOAT3 half = GetHalf();
		center.x = min.x + half.x;
		center.y = min.y + half.y;
		center.z = min.z + half.z;
		return center;
	}
};

struct Hit
{
	bool isHit;
	DirectX::XMFLOAT3 normal;
	float depth = 0.0f;
};

namespace CollisionSystem
{
	std::vector<AABB> AllColliders();
	void ClearColliders();

	void AddCollidersAABB(const AABB& aabb);

	bool ResolveAgainstScene(
		const IAABBProvider& playerAABBProvider,
		DirectX::XMFLOAT3& position,
		int maxIterations = 4,
		bool* outGround = nullptr
	);

};


bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b);
bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point);
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);
bool Collision_IsOverlapBox(const Box& a, const Box& b);
bool Collision_IsOverlapAABB(const AABB& a, const AABB& b);

Hit Collision_IsHitAABB(const AABB& a, const AABB& b);

AABB Collision_TransformAABB(const AABB& aabb, const DirectX::XMMATRIX& world);
AABB Collision_TranslateAABB(const AABB& aabb, const DirectX::XMFLOAT3& delta);

// Debug draw
void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_DebugFinalize();
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4 color = { 1.0f, 0.0f, 0.0f, 1.0f });
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4 color = { 1.0f, 0.0f, 0.0f, 1.0f });
void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4 color = { 1.0f, 0.0f, 0.0f, 1.0f });


#endif // COLLISION_H