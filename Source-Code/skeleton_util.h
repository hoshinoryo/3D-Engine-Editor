/*==============================================================================

   skeletonèÓïÒÇàµÇ§ã§í ÉwÉãÉpÅ[ [skeleton_util.h]
														 Author : Gu Anyi
														 Date   : 2025/11/14

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SKELETON_UTIL_H
#define SKELETON_UTIL_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <DirectXMath.h>

#include "model.h"

namespace SkeletonUtil
{
	using BoneNameSet = std::unordered_set<std::string>;
	using NodeSet = std::unordered_set<const aiNode*>;
	using NameToNodeMap = std::unordered_map<std::string, const aiNode*>;

	void BuildSkeletonNameSet(const aiScene* scene, BoneNameSet& outNames);
	void BuildNameToNodeMap(const aiNode* node, NameToNodeMap& map);
	void BuildSkeletonClosure(const aiScene* scene,
		const BoneNameSet& boneNames,
		NodeSet& outSkelSet);
	void FindSkeletonRoots(const aiScene* scene,
		const NodeSet& skelSet,
		std::vector<const aiNode*>& outRoots);
	bool AnyChildInSet(const aiNode* node, const NodeSet& S);

	const aiNode* FindNodeByName(const aiScene* scene, const std::string& name);

	void BuildBoneNameToIndexTable(const aiScene* scene, std::unordered_map<std::string, int>& outMap);
}


#endif // SKELETON_UTIL_H

