/*==============================================================================

   assimpライブラリ取り扱うヘルパー [assimp_util.h]
														 Author : Gu Anyi
														 Date   : 2026/02/04

--------------------------------------------------------------------------------

==============================================================================*/

#ifndef ASSIMP_UTIL_H
#define ASSIMP_UTIL_H

#include <DirectXMath.h>
#include <vector>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#include "assimp/config.h"
#pragma comment (lib, "assimp-vc143-mt.lib")


namespace AssimpUtil
{
	DirectX::XMMATRIX AiToXM(const aiMatrix4x4& m);
	void BuildNodeToModelRecursive(
		const aiNode* node,
		DirectX::CXMMATRIX parentAccum,
		std::vector<DirectX::XMFLOAT4X4>& outPerMesh
	);
	void BuildNodeToModelPerMesh(
		const aiScene* scene,
		DirectX::CXMMATRIX importFix,
		std::vector<DirectX::XMFLOAT4X4>& outPerMesh
	);
}

#endif // ASSIMP_UTIL_H