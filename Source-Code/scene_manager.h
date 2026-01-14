/*==============================================================================

   Scene Manager [scene_manager.h]
														 Author : Gu Anyi
														 Date   : 2025/11/10
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <vector>
#include <cstdint>
#include <DirectXMath.h>

#include "model.h"

struct SceneObject
{
	uint32_t id = 0; // 0 for none
	MODEL* model = nullptr;
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	bool visible = true;
	bool pickable = true;
};

namespace SceneManager
{
	uint32_t RegisterObject(MODEL* model, const DirectX::XMMATRIX& world, bool pickable = true);

	void UnregisterObject(uint32_t objectId);
	SceneObject* FindObject(uint32_t objectId);

	// Object list
	const std::vector<SceneObject>& AllObjects();

	// For outliner UI (models unique list)
	const std::vector<MODEL*>& AllModels();

	void Clear();
}

#endif // SCENE_MANAGER_H
