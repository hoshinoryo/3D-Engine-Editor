/*==============================================================================

   Model asset import and release [model_asset.h]
														 Author : Gu Anyi
														 Date   : 2026/01/28
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef MODEL_ASSET_H
#define MODEL_ASSET_H

// --------------------
// Assimp lib
// --------------------
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#include "assimp/config.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include <vector>
#include <unordered_map>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

#include "collision.h"

class Default3DMaterial;

struct Vertex3d
{
	DirectX::XMFLOAT3 position; // 頂点座標
	DirectX::XMFLOAT3 normal;   // 法線
	DirectX::XMFLOAT3 tangent;  // 切線
	DirectX::XMFLOAT4 color;    // 色
	DirectX::XMFLOAT2 texcoord; // UV

	UINT  boneIndex[4]; // 影響するボーンのインデックス
	float boneWeight[4];// 各ボーンのウェイト
};

// aiMeshごとに管理されてる
struct MeshAsset
{
	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
	uint32_t indexCount = 0;
	uint32_t materialIndex = 0;

	bool skinned = false;
	AABB localAABB{};
};

// fbxファイルごとに管理されている
struct ModelAsset
{
	// Import settings
	float importScale = 1.0f;
	bool sourceYup = true;

	DirectX::XMMATRIX importFix = DirectX::XMMatrixIdentity();

	// Assimp assets
	const aiScene* aiScene = nullptr;

	// GPU resources and materials
	std::vector<MeshAsset> meshes;
	std::unordered_map<std::string, ID3D11ShaderResourceView*> textures;
	std::vector<Default3DMaterial*> materials;

	// Bones
	std::unordered_map<std::string, int> boneNameToIndex;
};

ModelAsset* ModelAsset_Load(const char* filename, bool yUp = false, float scale = 1.0f);
void        ModelAsset_Release(ModelAsset* asset);

#endif // MODEL_ASSET_H
