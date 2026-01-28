/*==============================================================================

   Outliner API [outliner.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/09
--------------------------------------------------------------------------------

==============================================================================*/

#include <unordered_set>
#include <algorithm>
#include <string>
#include <vector>

#include "outliner.h"
#include "texture.h"
#include "skeleton_util.h"
#include "scene_manager.h"
#include "mesh_object.h"
#include "model_asset.h"

namespace Outliner
{
	struct DrawerRegistry
	{
		DrawProc mesh = nullptr;
		DrawProc skeleton = nullptr;
		DrawProc light = nullptr;
		DrawProc camera = nullptr;
		DrawProc material = nullptr;
		DrawProc custom1 = nullptr;
	};
	

	struct IconRegistry
	{
		Texture mesh;
		Texture skeleton;

		bool hasMesh = false;
		bool hasSkeleton = false;

		ImVec2 iconSize = ImVec2(16, 16);

		void Release()
		{
			mesh.Release();
			skeleton.Release();
			hasMesh = false;
			hasSkeleton = false;
			iconSize = ImVec2(16, 16);
		}
	};

	// ---- ïœêîêÈåæ ----
	static uint32_t g_selectedObjectId = 0;

	static DrawerRegistry g_drawers;
	static IconRegistry g_icons;

	static ID3D11ShaderResourceView* IconSRV(ViewKind kind);

	// Forward decl
	static bool NodeHasMeshes(const aiNode* node);
	static bool SubTreeHasMeshes(const aiScene* scene, const aiNode* node);
	static void MeshNodeRow(ModelAsset* asset, unsigned meshIdx);
	static void MeshNodeProjected(ModelAsset* asset, const aiNode* node);
	static void DrawSkeletonSubtree(const aiNode* node, const std::unordered_set<const aiNode*>& S);


	static void BuildNameToNodeMap(const aiNode* node, std::unordered_map<std::string, const aiNode*>& map)
	{
		if (!node) return;

		if (node->mName.length > 0)
			map.emplace(node->mName.C_Str(), node);

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			BuildNameToNodeMap(node->mChildren[i], map);
		}
	}


	bool InitIcons(const wchar_t* meshIconPath, const wchar_t* skeletonIconPath, ImVec2 iconSize)
	{
		g_icons.Release();

		if (meshIconPath && meshIconPath[0] != L'\0')
		{
			g_icons.hasMesh = g_icons.mesh.Load(meshIconPath);
		}
		if (skeletonIconPath && skeletonIconPath[0] != L'\0')
		{
			g_icons.hasSkeleton = g_icons.skeleton.Load(skeletonIconPath);
		}

		g_icons.iconSize = iconSize;

		return g_icons.hasMesh || g_icons.hasSkeleton;
	}

	void ShutdownIcons()
	{
		g_icons.Release();
	}

	static ID3D11ShaderResourceView* IconSRV(ViewKind kind)
	{
		switch (kind)
		{
		case Outliner::ViewKind::MESH:
			return g_icons.hasMesh ? g_icons.mesh.GetSRV() : nullptr;
			break;
		case Outliner::ViewKind::SKELETON:
			return g_icons.hasSkeleton ? g_icons.skeleton.GetSRV() : nullptr;
		case Outliner::ViewKind::LIGHT:
		case Outliner::ViewKind::CAMERA:
		case Outliner::ViewKind::MATERIAL:
		case Outliner::ViewKind::CUSTOM1:
		default:
			return nullptr;
		}
	}

	void DrawIcon(ViewKind kind)
	{
		if (ID3D11ShaderResourceView* srv = IconSRV(kind))
		{
			ImGui::Image((ImTextureID)srv, g_icons.iconSize);
			ImGui::SameLine(0.0f, 4.0f);
		}
	}

	// UI entry
	void ShowSceneOutliner()
	{
		const auto& assets = SceneManager::AllModelAssets();

		for (size_t ai = 0; ai < assets.size(); ++ai)
		{
			ModelAsset* asset = assets[ai];
			if (!asset || !asset->aiScene || !asset->aiScene->mRootNode) continue;

			ImGui::PushID((int)ai);

			const char* title = "(asset)"; // default name label

			if (asset->aiScene->mRootNode->mName.length > 0)
			{
				title = asset->aiScene->mRootNode->mName.C_Str();
			}

			bool open = ImGui::TreeNodeEx(
				title,
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick
			);

			// Show/Hide All button only for root node
			ImGui::SameLine();
			if (ImGui::SmallButton("Show All"))
			{
				SceneManager::SetVisibleByAsset(asset, true);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Hide All"))
			{
				SceneManager::SetVisibleByAsset(asset, false);
			}

			if (open)
			{
				// Mesh hierarchy
				MeshNodeProjected(asset, asset->aiScene->mRootNode);

				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	}

	bool GetSelection(uint32_t& objectId)
	{
		objectId = g_selectedObjectId;

		return objectId != 0;
	}


	// ---- Mesh node drawing ----
	static void MeshNodeRow(ModelAsset* asset, unsigned meshIdx)
	{
		if (!asset || !asset->aiScene) return;
		if (meshIdx >= asset->aiScene->mNumMeshes) return;

		aiMesh* mesh = asset->aiScene->mMeshes[meshIdx];
		if (!mesh) return;

		MeshObject* obj = SceneManager::FindByAssetMesh(asset, meshIdx);

		ImGui::PushID((int)meshIdx);

		const bool hasObject = (obj != nullptr);
		bool vis = hasObject ? obj->visible : true;

		if (!hasObject) ImGui::BeginDisabled(true);

		if (ImGui::Checkbox("##vis", &vis))
		{
			if (hasObject) obj->visible = vis;
		}
		if (!hasObject) ImGui::EndDisabled();

		ImGui::SameLine();

		// Icon
		DrawIcon(ViewKind::MESH);

		// Mesh label
		std::string meshLabel = (mesh->mName.length > 0) ?
			mesh->mName.C_Str() :
			("mesh[" + std::to_string(meshIdx) + "]");

		const bool selected = hasObject && (g_selectedObjectId == obj->id);
		if (ImGui::Selectable(meshLabel.c_str(), selected))
		{
			g_selectedObjectId = obj->id;
		}

		ImGui::SameLine();
		ImGui::TextDisabled(" V: %u F:%u", mesh->mNumVertices, mesh->mNumFaces);

		if (!hasObject) ImGui::EndDisabled();

		ImGui::PopID();
	}
	
	// Hierarcky projection for mesh node
	static void MeshNodeProjected(ModelAsset* asset, const aiNode* node)
	{
		if (!asset || !asset->aiScene || !node) return;
		if (!SubTreeHasMeshes(asset->aiScene, node)) return;

		ImGui::PushID(node);

		const char* nodeLabel = (node->mName.length > 0) ? node->mName.C_Str() : "(node)";
		const bool hasChildren = (node->mNumChildren > 0);
		const bool hasMeshes = NodeHasMeshes(node);

		if (!hasChildren && hasMeshes) // leaf
		{
			for (unsigned int i = 0; i < node->mNumMeshes; ++i)
			{
				MeshNodeRow(asset, node->mMeshes[i]);
			}
		}
		else
		{
			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick;

			bool open = ImGui::TreeNodeEx(nodeLabel, flags);

			if (open)
			{
				for (unsigned int c = 0; c < node->mNumChildren; ++c)
				{
					MeshNodeProjected(asset, node->mChildren[c]);
				}

				ImGui::TreePop();
			}
		}

		ImGui::PopID();
	}

	void DrawMeshNode(ModelAsset* asset)
	{
		if (!asset || !asset->aiScene || !asset->aiScene->mRootNode) return;

		MeshNodeProjected(asset, asset->aiScene->mRootNode);
	}

	void DrawSkeletonNode(ModelAsset* asset)
	{
		if (!asset || !asset->aiScene || !asset->aiScene->mRootNode) return;

		// 1. Collect all binded bones
		SkeletonUtil::BoneNameSet boneNames;
		SkeletonUtil::BuildSkeletonNameSet(asset->aiScene, boneNames);
		if (boneNames.empty()) return;

		// 2. Build closure S(bones and their parents, without mRootNode)
		SkeletonUtil::NodeSet skelSet;
		SkeletonUtil::BuildSkeletonClosure(asset->aiScene, boneNames, skelSet);
		if (skelSet.empty()) return;

		// 3. Find all skeleton tree roots
		std::vector<const aiNode*> roots;
		SkeletonUtil::FindSkeletonRoots(asset->aiScene, skelSet, roots);

		// 4. Draw skeleton hierarchy
		for (const aiNode* r : roots)
		{
			DrawSkeletonSubtree(r, skelSet);
		}
	}

	static bool NodeHasMeshes(const aiNode* node)
	{
		return node && node->mNumMeshes > 0;
	}

	static bool SubTreeHasMeshes(const aiScene* scene, const aiNode* node)
	{
		if (!scene || !node) return false;
		if (NodeHasMeshes(node)) return true;
		for (unsigned i = 0; i < node->mNumChildren; ++i)
		{
			if (SubTreeHasMeshes(scene, node->mChildren[i])) return true;
		}

		return false;
	}


	// ---- Skeleton node drawing ----
	static void DrawSkeletonSubtree(const aiNode* node, const std::unordered_set<const aiNode*>& S)
	{
		if (!node || !S.count(node)) return;

		ImGui::PushID(node);

		const bool hasChild = SkeletonUtil::AnyChildInSet(node, S);
		const char* label = (node->mName.length > 0) ? node->mName.C_Str() : "(skeleton)";

		if (!hasChild) // leave
		{
			DrawIcon(ViewKind::SKELETON);
			ImGui::SameLine();
			ImGui::TextUnformatted(label);
		}
		else
		{
			DrawIcon(ViewKind::SKELETON);
			ImGui::SameLine(0.0f, 0.0f);

			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick;

			const bool open = ImGui::TreeNodeEx(label, flags);
			if (open)
			{
				for (unsigned int i = 0; i < node->mNumChildren; ++i)
				{
					const aiNode* ch = node->mChildren[i];
					if (S.count(ch))
						DrawSkeletonSubtree(ch, S);
				}
				ImGui::TreePop();
			}
		}
		ImGui::PopID();
	}
}
