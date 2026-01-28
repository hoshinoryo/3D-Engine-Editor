/*==============================================================================

   Outliner API [outliner.h]
														 Author : Gu Anyi
														 Date   : 2025/11/09
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef OUTLINER_H
#define OUTLINER_H

#include <cstdint>

#include "imgui/imgui.h"

struct ModelAsset;

namespace Outliner
{
	enum class ViewKind
	{
		MESH,
		SKELETON,
		LIGHT,
		CAMERA,
		MATERIAL,
		CUSTOM1
	};

	using DrawProc = void(*)(const ModelAsset* asset);

	// Icon function
	bool InitIcons(
		const wchar_t* meshIconPath,
		const wchar_t* skeletonIconPath,
		ImVec2 iconSize = ImVec2(16, 16)
	);
	void ShutdownIcons();
	void DrawIcon(ViewKind kind);

	// Show Outliner
	void ShowSceneOutliner();

	bool GetSelection(uint32_t& objectId);

	// ---- Draw Outliner ----

	// Mesh
	void DrawMeshNode(const ModelAsset* asset);

	// Skeleton
	void DrawSkeletonNode(const ModelAsset* asset);
}

#endif // !OUTLINER_H
