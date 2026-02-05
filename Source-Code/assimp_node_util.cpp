/*==============================================================================

   assimpライブラリ取り扱うヘルパー [assimp_util.h]
														 Author : Gu Anyi
														 Date   : 2026/02/04

--------------------------------------------------------------------------------

==============================================================================*/

#include "assimp_node_util.h"

using namespace DirectX;

// aiMatrix4x4 to XMMATRIX
XMMATRIX AssimpUtil::AiToXM(const aiMatrix4x4& m)
{
	XMMATRIX x{
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	};

	return x;
}

void AssimpUtil::BuildNodeToModelRecursive(
	const aiNode* node,
	CXMMATRIX parentAccum,
	std::vector<XMFLOAT4X4>& outPerMesh
)
{
	if (!node) return;

	XMMATRIX local = AiToXM(node->mTransformation);
	XMMATRIX accum = local * parentAccum;

	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		if (meshIndex < outPerMesh.size())
		{
			XMStoreFloat4x4(&outPerMesh[meshIndex], accum);
		}
	}

	for (unsigned int c = 0; c < node->mNumChildren; c++)
	{
		BuildNodeToModelRecursive(node->mChildren[c], accum, outPerMesh);
	}
}

// include importFix
void AssimpUtil::BuildNodeToModelPerMesh(
	const aiScene* scene,
	CXMMATRIX importFix,
	std::vector<XMFLOAT4X4>& outPerMesh
)
{
	outPerMesh.clear();
	if (!scene || !scene->mNumMeshes) return;

	outPerMesh.resize(scene->mNumMeshes);

	// initialize matrix
	BuildNodeToModelRecursive(scene->mRootNode, importFix, outPerMesh);
}
