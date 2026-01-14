/*==============================================================================

   Scene Manager [scene_manager.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/10
--------------------------------------------------------------------------------

==============================================================================*/

#include "scene_manager.h"

#include <algorithm>

namespace
{
	std::vector<SceneObject> g_objects;
	std::vector<MODEL*> g_modelsUniqueCache;
	bool g_modelsDirty = true;

	uint32_t g_nextId = 1;

	void MarkModelsDirty() { g_modelsDirty = true; }

	void RebuildModelsCache()
	{
		g_modelsUniqueCache.clear();

		for (auto& o : g_objects)
		{
			if (!o.model) continue;
			if (std::find(g_modelsUniqueCache.begin(), g_modelsUniqueCache.end(), o.model) == g_modelsUniqueCache.end())
				g_modelsUniqueCache.push_back(o.model);
		}

		g_modelsDirty = false;
	}
}

namespace SceneManager
{
	uint32_t RegisterObject(MODEL* model, const DirectX::XMMATRIX& world, bool pickable)
	{
		if (!model) return 0;

		SceneObject o;
		o.id = g_nextId++;
		o.model = model;
		o.world = world;
		o.pickable = pickable;

		g_objects.push_back(o);
		MarkModelsDirty();

		return o.id;
	}

	void UnregisterObject(uint32_t objectId)
	{
		auto it = std::remove_if(g_objects.begin(), g_objects.end(),
			[objectId](const SceneObject& o) { return o.id == objectId; });
		if (it != g_objects.end())
		{
			g_objects.erase(it, g_objects.end());
			MarkModelsDirty();
		}
	}

	SceneObject* FindObject(uint32_t objectId)
	{
		for (auto& o : g_objects)
		{
			if (o.id == objectId) return &o;
		}
		
		return nullptr;
	}

	const std::vector<SceneObject>& AllObjects()
	{
		return g_objects;
	}

	const std::vector<MODEL*>& AllModels()
	{
		if (g_modelsDirty) RebuildModelsCache();
		return g_modelsUniqueCache;
	}

	void Clear()
	{
		g_objects.clear();
		g_modelsUniqueCache.clear();
		g_modelsDirty = false;
		g_nextId = 1;
	}
}

