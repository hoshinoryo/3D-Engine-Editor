#pragma once

#include <unordered_map>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#include "assimp/config.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

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

struct MODEL
{
	float Scale = 1.0f;
	bool SourceYup = true;
	AABB localAABB{};
	AABB worldAABB{};

	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer = nullptr;
	ID3D11Buffer** IndexBuffer = nullptr;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	std::vector<Default3DMaterial*> Materials;

	std::unordered_map<std::string, int> BoneNameToIndex;
};


MODEL* ModelLoad(const char* FileName, bool yUp = true, float scale = 1.0f);
void ModelRelease(MODEL* model);

void ModelDraw(MODEL* model,
	const DirectX::XMMATRIX& mtxWorld,
	const DirectX::XMFLOAT3& cameraPos
);
void ModelUnlitDraw(MODEL* model,
	const DirectX::XMMATRIX& mtxWorld,
	const DirectX::XMFLOAT4& color
);

float ModelGetScale(const MODEL* model);

