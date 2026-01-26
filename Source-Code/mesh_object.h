#ifndef MESH_OBJECT_H
#define MESH_OBJECT_H

#include <cstdint>
#include <string>

#include "transform.h"

struct ModelAsset;

struct MeshObjectSerializable
{
	std::string assetPath;  // path
	uint32_t meshIndex = 0; // meshId
	TransformTRS trs;       // TRS data
};

struct MeshObject
{
	uint32_t id = 0;

	ModelAsset* asset = nullptr;
	uint32_t meshIndex = 0;

	TransformTRS transform;

	bool visible = true;
	bool pickable = true;

	std::string name;
};

#endif // MESH_OBJECT_H
