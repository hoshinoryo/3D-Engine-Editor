/*==============================================================================

   skeletonの描画 [skeleton.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/14

--------------------------------------------------------------------------------

==============================================================================*/

#include "skeleton.h"
#include "skeleton_util.h"
#include "axis_util.h"
//#include "model.h"
#include "model_asset.h"
#include "scene_manager.h"
#include "draw3d.h"
#include "animation.h"

#include <DirectXMath.h>
#include <unordered_map>
#include <unordered_set>

using namespace DirectX;

struct SkeletonSettings
{
	float jointSize = 0.2f;
	XMFLOAT4 jointColor = { 1.0f, 0.9f, 0.2f, 1.0f };
	XMFLOAT4 boneColor = { 0.9f, 0.6f, 0.1f, 1.0f };
};

SkeletonSettings g_settings;
extern AnimationPlayer g_AnimPlayer;

// Helpers
static XMMATRIX AiMatToXM(const aiMatrix4x4& m);
static void BuildNodeWorldMatrices(const aiNode* node, const XMMATRIX& parentWorld, std::unordered_map<const aiNode*, XMMATRIX>& nodeWorlds);

// Skelton Drawing Function
static void DrawSkeletonForAsset(const ModelAsset* asset, const XMMATRIX& world);
static void DrawAnimatedSkeletonForAsset(const ModelAsset* asset, const AnimationPlayer* player, const XMMATRIX& world);


void Skeleton_Initialize()
{
	g_settings = SkeletonSettings();
}

void Skeleton_Finalize()
{
}

void Skeleton_Draw()
{
	const auto& objects = SceneManager::AllObjects();

	for (const auto& obj : objects)
	{
		if (!obj.visible) continue;
		if (!obj.asset || !obj.asset->aiScene) continue;

		const XMMATRIX world = obj.transform.ToMatrix();

		if (g_AnimPlayer.IsPLaying() && g_AnimPlayer.GetAsset() == obj.asset)
		{
			DrawAnimatedSkeletonForAsset(obj.asset, &g_AnimPlayer, world);
		}
		else
		{
			DrawSkeletonForAsset(obj.asset, world);
		}
	}
}

static XMMATRIX AiMatToXM(const aiMatrix4x4& m)
{
	return XMMATRIX(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
};

// 各aiNodeのワールド行列を再帰的に計算して保存する
static void BuildNodeWorldMatrices(
	const aiNode* node,
	const XMMATRIX& parentWorld,
	std::unordered_map<const aiNode*, XMMATRIX>& nodeWorlds
)
{
	if (!node) return;

	XMMATRIX local = AiMatToXM(node->mTransformation);
	XMMATRIX world = local * parentWorld;

	nodeWorlds[node] = world;

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		BuildNodeWorldMatrices(node->mChildren[i], world, nodeWorlds);
	}
}

static void DrawSkeletonForAsset(const ModelAsset* asset, const XMMATRIX& world)
{
	if (!asset || !asset->aiScene || !asset->aiScene->mRootNode) return;
	const aiScene* scene = asset->aiScene;

	// 1. Collect all binded bones
	SkeletonUtil::BoneNameSet boneNames;
	SkeletonUtil::BuildSkeletonNameSet(scene, boneNames);
	if (boneNames.empty()) return;

	// 2. Build closure S(bones and their parents, without mRootNode)
	SkeletonUtil::NodeSet skelSet;
	SkeletonUtil::BuildSkeletonClosure(scene, boneNames, skelSet);
	if (skelSet.empty()) return;

	// 3. Compute world matrix for each aiNode(model local space)
	std::unordered_map<const aiNode*, XMMATRIX> nodeWorlds;
	BuildNodeWorldMatrices(scene->mRootNode, XMMatrixIdentity(), nodeWorlds);

	// ワールド行列
	const XMMATRIX finalWorld = asset->importFix * world;

	// 4. Draw joints and bones
	for (const aiNode* n : skelSet)
	{
		auto itW = nodeWorlds.find(n);
		if (itW == nodeWorlds.end()) continue;

		XMMATRIX jointWorld = itW->second * finalWorld;

		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, jointWorld);
		XMFLOAT3 jointPos(m._41, m._42, m._43);

		// Draw joint
		Draw3d_MakeCross(jointPos, g_settings.jointSize, g_settings.jointColor);

		// Draw Bone
		const aiNode* parent = n->mParent;
		if (parent && skelSet.count(parent))
		{
			auto itPW = nodeWorlds.find(parent);
			if (itPW != nodeWorlds.end())
			{
				XMMATRIX parentWorld = itPW->second * finalWorld;

				XMFLOAT4X4 mp;
				XMStoreFloat4x4(&mp, parentWorld);
				XMFLOAT3 parentPos(mp._41, mp._42, mp._43);

				Draw3d_MakeLine(parentPos, jointPos, g_settings.boneColor);
			}
		}
	}
}

static void DrawAnimatedSkeletonForAsset(const ModelAsset* asset, const AnimationPlayer* player, const XMMATRIX& world)
{
	if (!asset || !asset->aiScene || !asset->aiScene->mRootNode) return;
	
	const aiScene* scene = asset->aiScene;

	// 1. Collect all binded bones
	SkeletonUtil::BoneNameSet boneNames;
	SkeletonUtil::BuildSkeletonNameSet(scene, boneNames);
	if (boneNames.empty()) return;

	// 2. Build closure S(bones and their parents, without mRootNode)
	SkeletonUtil::NodeSet skelSet;
	SkeletonUtil::BuildSkeletonClosure(scene, boneNames, skelSet);
	if (skelSet.empty()) return;

	// 3. Get pose from AnimationPlayer
	std::unordered_map<const aiNode*, XMMATRIX> nodeWorlds;
	player->GetCurrentPose(nodeWorlds);

	const XMMATRIX finalWorld = asset->importFix * world;

	// 4. Draw joints and bones
	for (const aiNode* n : skelSet)
	{
		auto itW = nodeWorlds.find(n);
		if (itW == nodeWorlds.end()) continue;

		XMMATRIX jointWorld = itW->second * finalWorld;

		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, jointWorld);
		XMFLOAT3 jointPos(m._41, m._42, m._43);

		// Draw joint
		Draw3d_MakeCross(jointPos, g_settings.jointSize, g_settings.jointColor);

		// Draw Bone
		const aiNode* parent = n->mParent;
		if (parent && skelSet.count(parent))
		{
			auto itPW = nodeWorlds.find(parent);
			if (itPW != nodeWorlds.end())
			{
				XMMATRIX parentWorld = itPW->second * finalWorld;

				XMFLOAT4X4 mp;
				XMStoreFloat4x4(&mp, parentWorld);
				XMFLOAT3 parentPos(mp._41, mp._42, mp._43);

				Draw3d_MakeLine(parentPos, jointPos, g_settings.boneColor);
			}
		}
	}
}

