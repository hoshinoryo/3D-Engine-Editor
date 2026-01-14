/*==============================================================================

   skeletonèÓïÒÇàµÇ§ã§í ÉwÉãÉpÅ[ [skeleton_util.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/14

--------------------------------------------------------------------------------

==============================================================================*/

#include "skeleton_util.h"

namespace SkeletonUtil
{
	// Collect all bone names from the scene's meshes
	// These names come from aiMesh::mBones[] and represent all bones used for skinning
	void SkeletonUtil::BuildSkeletonNameSet(const aiScene* scene, BoneNameSet& outNames)
	{
		outNames.clear();

		if (!scene) return;

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			const aiMesh* mesh = scene->mMeshes[m];
			if (!mesh) continue;

			for (unsigned int b = 0; b < mesh->mNumBones; ++b)
			{
				const aiBone* bone = mesh->mBones[b];
				if (!bone) continue;

				if (bone->mName.length > 0)
					outNames.insert(bone->mName.C_Str());
			}
		}
	}

	// Build a lookup table: node-name Å® aiNode pointer.
	// This function walks through the whole aiNode hierarchy (recursive)
	// and inserts every node into an unordered_map.
	void SkeletonUtil::BuildNameToNodeMap(const aiNode* node, NameToNodeMap& map)
	{
		if (!node) return;

		if (node->mName.length > 0)
			map.emplace(node->mName.C_Str(), node);

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			BuildNameToNodeMap(node->mChildren[i], map);
		}
	}

	// Build a set of skeleton-related nodes (closure)
	// For each bone name:
	//   1. Find the aiNode with the same name
	//   2. Add that node to the skeleton set
	//   3. Walk up its parent chain and add all ancestors
	void SkeletonUtil::BuildSkeletonClosure(const aiScene* scene, const BoneNameSet& boneNames, NodeSet& outSkelSet)
	{
		outSkelSet.clear();

		if (!scene || !scene->mRootNode) return;

		// Name to node dictionary
		NameToNodeMap name2node;
		BuildNameToNodeMap(scene->mRootNode, name2node);

		for (const auto& nm : boneNames)
		{
			auto it = name2node.find(nm);
			if (it == name2node.end()) continue;

			const aiNode* n = it->second;

			while (n)
			{
				if (n == scene->mRootNode) break;
				if (!outSkelSet.insert(n).second) break;
				n = n->mParent;
			}
		}
	}

	// Find root nodes of the skeleton hierarchy
	void SkeletonUtil::FindSkeletonRoots(const aiScene* scene, const NodeSet& skelSet, std::vector<const aiNode*>& outRoots)
	{
		outRoots.clear();

		if (!scene) return;
		for (const aiNode* n : skelSet)
		{
			const aiNode* p = n->mParent;

			if (p == nullptr || p == scene->mRootNode || skelSet.find(p) == skelSet.end())
			{
				outRoots.push_back(n);
			}
		}
	}

	// Check if this aiNode has at least one child inside the skeleton set S
	// If true Å® this node should be shown as a TreeNode
	// If false Å® this node is a leaf
	bool AnyChildInSet(const aiNode* node, const NodeSet& S)
	{
		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			if (S.count(node->mChildren[i])) return true;
		}

		return false;
	}

	const aiNode* FindNodeByName(const aiScene* scene, const std::string& name)
	{
		if (!scene || !scene->mRootNode) return nullptr;

		NameToNodeMap map;
		BuildNameToNodeMap(scene->mRootNode, map);

		auto it = map.find(name);
		if (it != map.end())
			return it->second;

		return nullptr;
	}

	void BuildBoneNameToIndexTable(const aiScene* scene, std::unordered_map<std::string, int>& outMap)
	{
		outMap.clear();

		if (!scene) return;

		int nextIndex = 0;

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			const aiMesh* mesh = scene->mMeshes[m];
			if (!mesh || mesh->mNumBones == 0) continue;

			for (unsigned int b = 0; b < mesh->mNumBones; ++b)
			{
				const aiBone* bone = mesh->mBones[b];
				if (!bone) continue;

				std::string boneName = bone->mName.C_Str();

				if (outMap.find(boneName) == outMap.end())
				{
					outMap[boneName] = nextIndex;
					++nextIndex;
				}
			}
		}
	}

}

