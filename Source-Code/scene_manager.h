/*==============================================================================

   Mesh object list management [scene_manager.h]
														 Author : Gu Anyi
														 Date   : 2025/11/10
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <vector>
#include <cstdint>

#include "model_asset.h"
#include "mesh_object.h"

struct AABB;
class CubeObject;

namespace SceneManager
{
	uint32_t RegisterMeshObject(ModelAsset* asset,uint32_t meshIndex, const TransformTRS& trs, bool pickable = true);
	void UnregisterMeshObject(uint32_t objectId);

	MeshObject* FindMeshObject(uint32_t objectId);
	MeshObject* FindByAssetMesh(ModelAsset* asset, uint32_t meshIndex);

	const std::vector<MeshObject>& AllObjects();
	const std::vector<ModelAsset*>& AllModelAssets();
	std::vector<MeshObject>& AllObjectsMutable();

	void SetVisibleByAsset(ModelAsset* asset, bool visible);
	void SetPickableByAsset(ModelAsset* asset, bool pickable);

	void UpdateWorldAABBs();

	void Clear();
}

#endif // SCENE_MANAGER_H
