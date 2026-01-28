/*==============================================================================

   Mesh object list management [scene_manager.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/10
--------------------------------------------------------------------------------

==============================================================================*/

#include "scene_manager.h"

#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

namespace
{
	std::vector<MeshObject> g_meshObjects;
	std::vector<ModelAsset*> g_assetsUniqueCache;
	bool g_assetsDirty = true;
	uint32_t g_nextId = 1;

	void MarkAssetsDirty()
	{
		g_assetsDirty = true; // asset list need to rebuild
	}

	void RebuildAssetsCache()
	{
		g_assetsUniqueCache.clear();

		for (auto& o : g_meshObjects)
		{
			if (!o.asset) continue;

			auto it = std::find(g_assetsUniqueCache.begin(), g_assetsUniqueCache.end(), o.asset);
			if (it == g_assetsUniqueCache.end())
				g_assetsUniqueCache.push_back(o.asset);
		}

		g_assetsDirty = false;
	}
}

namespace SceneManager
{
	uint32_t RegisterMeshObject(ModelAsset* asset, uint32_t meshIndex, const TransformTRS& trs, bool pickable)
	{
		if (!asset) return 0;
		if (meshIndex >= asset->meshes.size()) return 0;

		MeshObject o;
		o.id = g_nextId++;
		o.asset = asset;
		o.meshIndex = meshIndex;
		o.transform = trs;
		o.visible = true;
		o.pickable = pickable;

		o.name = "Mesh_" + std::to_string(meshIndex);

		g_meshObjects.push_back(o);
		MarkAssetsDirty();

		return o.id;
	}

	void UnregisterMeshObject(uint32_t objectId)
	{
		auto it = std::remove_if(
			g_meshObjects.begin(),
			g_meshObjects.end(),
			[objectId](const MeshObject& o) { return o.id == objectId; }
		);
		if (it != g_meshObjects.end())
		{
			g_meshObjects.erase(it, g_meshObjects.end());
			MarkAssetsDirty();
		}
	}

	MeshObject* FindMeshObject(uint32_t objectId)
	{
		for (auto& o : g_meshObjects)
		{
			if (o.id == objectId) return &o;
		}
		return nullptr;
	}

	MeshObject* FindByAssetMesh(ModelAsset* asset, uint32_t meshIndex)
	{
		for (auto& o : g_meshObjects)
		{
			if (o.asset == asset && o.meshIndex == meshIndex)
				return &o;
		}
		return nullptr;
	}

	// for mesh
	const std::vector<MeshObject>& AllObjects()
	{
		return g_meshObjects;
	}

	// for outliner (model asset)
	const std::vector<ModelAsset*>& AllModelAssets()
	{
		if (g_assetsDirty) RebuildAssetsCache();
		return g_assetsUniqueCache;
	}

	// mutable meshes
	std::vector<MeshObject>& AllObjectsMutable()
	{
		return g_meshObjects;
	}

	void SetVisibleByAsset(ModelAsset* asset, bool visible)
	{
		for (auto& o : g_meshObjects)
		{
			if (o.asset == asset)
				o.visible = visible;
		}
	}

	void SetPickableByAsset(ModelAsset* asset, bool pickable)
	{
		for (auto& o : g_meshObjects)
		{
			if (o.asset == asset)
				o.pickable = pickable;
		}
	}

	void UpdateWorldAABBs()
	{
		for (auto& obj : AllObjectsMutable())
		{
			if (!obj.asset)
			{
				obj.aabbValid = false;
				continue;
			}

			const XMMATRIX world = obj.transform.ToMatrix();
			const XMMATRIX finalWorld = obj.asset->importFix * world;

			obj.worldAABB = Collision_TransformAABB(obj.asset->meshes[obj.meshIndex].localAABB, finalWorld);
			obj.aabbValid = true;
		}
	}

	void Clear()
	{
		g_meshObjects.clear();
		g_assetsUniqueCache.clear();
		g_assetsDirty = false;
		g_nextId = 1;
	}
}

