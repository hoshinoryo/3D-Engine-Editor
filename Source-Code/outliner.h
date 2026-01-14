/*==============================================================================

   Outliner API [outliner.h]
														 Author : Gu Anyi
														 Date   : 2025/11/09
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef OUTLINER_H
#define OUTLINER_H

#include <vector>
#include <unordered_map>
#include <string>

#include "model.h"
#include "imgui/imgui.h"

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

	using DrawProc = void(*)(const MODEL* model, const aiScene* scene);
	
	// Drawer reigister function
	void SetDrawer(ViewKind kind, DrawProc fn);
	void InitDefaultDrawers();

	// Icon function
	bool InitIcons(
		const wchar_t* meshIconPath,
		const wchar_t* skeletonIconPath,
		ImVec2 iconSize = ImVec2(16, 16)
	);
	void ShutdownIcons();
	ID3D11ShaderResourceView* IconSRV(ViewKind kind); // get icon from the type
	void DrawIcon(ViewKind kind);

	// Show Outliner
	void ShowSceneOutliner(const std::vector<MODEL*>& models);

	bool GetSelection(const MODEL*& model, int& meshIndex);
	bool IsMeshVisible(const MODEL* model, int meshIndex);
	void SetMeshVisible(const MODEL* model, int meshIndex, bool visible);

	// ---- Draw Outliner ----
	void DrawAiNode(const MODEL* model, const aiScene* scene, const aiNode* node);

	// Mesh
	void DrawMeshNode(const MODEL* model, const aiScene* scene);

	// Skeleton
	void DrawSkeletonNode(const MODEL* model, const aiScene* scene);
	
}



#endif // !OUTLINER_H
