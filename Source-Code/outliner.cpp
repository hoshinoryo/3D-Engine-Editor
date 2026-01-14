/*==============================================================================

   Outliner API [outliner.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/09
--------------------------------------------------------------------------------

==============================================================================*/

#include "outliner.h"
#include "texture.h"
#include "skeleton_util.h"

#include <cassert>
#include <unordered_set>

namespace Outliner
{
	// Model status
	struct PerModelState
	{
		std::vector<bool> meshVisible; // visibility for each mesh
	};

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
	static std::unordered_map<const MODEL*, PerModelState> g_state; // unordered_map = dictionary

	static const MODEL* g_selectedModel = nullptr;
	static int g_selectedMesh = -1;

	static DrawerRegistry g_drawers;
	static IconRegistry g_icons;

	// ---- Inner tool function ----
	//static void BuildNameToNodeMap(const aiNode* node, std::unordered_map<std::string, const aiNode*>& map);
	static PerModelState& ensureState(const MODEL* model);

	static bool NodeHasMeshes(const aiNode* node);
	static bool SubTreeHasMeshes(const aiScene* scene, const aiNode* node);
	static void MeshNodeRow(const MODEL* model, const aiScene* scene, unsigned meshIdx);
	static void MeshNodeProjected(const MODEL* model, const aiScene* scene, const aiNode* node);


	inline void DispatchAllRegisteredDrawers(const MODEL* model, const aiScene* scene)
	{
		if (g_drawers.mesh)      g_drawers.mesh(model, scene);
		if (g_drawers.skeleton)  g_drawers.skeleton(model, scene);
		if (g_drawers.light)     g_drawers.light(model, scene);
		if (g_drawers.camera)    g_drawers.camera(model, scene);
		if (g_drawers.material)  g_drawers.material(model, scene);
		if (g_drawers.custom1)   g_drawers.custom1(model, scene);
	}

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

	static PerModelState& ensureState(const MODEL* model)
	{
		auto it = g_state.find(model);

		if (it == g_state.end())
		{
			// Model doesn't exist
			PerModelState st;

			if (model && model->AiScene)
			{
				st.meshVisible.assign(model->AiScene->mNumMeshes, true);
			}
			it = g_state.emplace(model, std::move(st)).first;
		}
		else
		{
			// Found the model
			if (model && model->AiScene)
			{
				auto& vis = it->second.meshVisible;

				if (vis.size() != model->AiScene->mNumMeshes)
				{
					vis.assign(model->AiScene->mNumMeshes, true);
				}
			}
		}

		return it->second;
	}

	void SetDrawer(ViewKind kind, DrawProc fn)
	{
		switch (kind)
		{
		case Outliner::ViewKind::MESH:
			g_drawers.mesh = fn;
			break;
		case Outliner::ViewKind::SKELETON:
			g_drawers.skeleton = fn;
			break;
		case Outliner::ViewKind::LIGHT:
			g_drawers.light = fn;
			break;
		case Outliner::ViewKind::CAMERA:
			g_drawers.camera = fn;
			break;
		case Outliner::ViewKind::MATERIAL:
			g_drawers.material = fn;
			break;
		case Outliner::ViewKind::CUSTOM1:
			g_drawers.custom1 = fn;
			break;
		}
	}

	void InitDefaultDrawers()
	{
		extern void DrawMeshNode(const MODEL*, const aiScene*);
		extern void DrawSkeletonNode(const MODEL*, const aiScene*);

		SetDrawer(ViewKind::MESH, DrawMeshNode);
		SetDrawer(ViewKind::SKELETON, DrawSkeletonNode);
	}

	bool InitIcons(
		const wchar_t* meshIconPath,
		const wchar_t* skeletonIconPath,
		ImVec2 iconSize
	)
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

	ID3D11ShaderResourceView* IconSRV(ViewKind kind)
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

	// fbxÉtÉ@ÉCÉãÇ≤Ç∆Ç…èàóù
	void Outliner::ShowSceneOutliner(const std::vector<MODEL*>& models)
	{
		for (size_t mi = 0; mi < models.size(); ++mi)
		{
			const MODEL* model = models[mi];

			if (!model || !model->AiScene || !model->AiScene->mRootNode) continue;

			ensureState(model);

			ImGui::PushID((int)mi);

			const char* title = "(model)"; // default name label

			if (model->AiScene->mRootNode->mName.length > 0)
			{
				title = model->AiScene->mRootNode->mName.C_Str();
			}

			bool openModel = ImGui::TreeNodeEx(
				title,
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick
			);

			// Show/Hide All button only for root node
			ImGui::SameLine();
			if (ImGui::SmallButton("Show All"))
			{
				auto& st = ensureState(model);
				std::fill(st.meshVisible.begin(), st.meshVisible.end(), true);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Hide All"))
			{
				auto& st = ensureState(model);
				std::fill(st.meshVisible.begin(), st.meshVisible.end(), false);
			}

			if (openModel)
			{
				//DrawMeshNode(model, model->AiScene);
				DispatchAllRegisteredDrawers(model, model->AiScene);

				ImGui::TreePop();
			}

			ImGui::PopID();

		}
	}

	bool Outliner::GetSelection(const MODEL*& model, int& meshIndex)
	{
		model = g_selectedModel;
		meshIndex = g_selectedMesh;

		return (model != nullptr && meshIndex >= 0);
	}

	bool Outliner::IsMeshVisible(const MODEL* model, int meshIndex)
	{
		if (!model || !model->AiScene)
		{
			return true;
		}

		auto& st = ensureState(model);

		if (meshIndex < 0 || (size_t)meshIndex >= st.meshVisible.size())
		{
			return true;
		}

		//return st.meshVisible[meshIndex];
		return st.meshVisible.at(meshIndex);
	}

	void Outliner::SetMeshVisible(const MODEL* model, int meshIndex, bool visible)
	{
		if (!model || !model->AiScene) return;

		auto& st = ensureState(model);

		if (meshIndex < 0 || (size_t)meshIndex >= st.meshVisible.size()) return;

		//st.meshVisible[meshIndex] = visible;
		st.meshVisible.at(meshIndex) = visible;
	}

	// NodeÇÃï`âÊ
	void Outliner::DrawAiNode(const MODEL* model, const aiScene* scene, const aiNode* node)
	{
		if (!scene || !node) return;

		ImGui::PushID(node);

		// node label
		const char* nodeLabel = (node->mName.length > 0) ? node->mName.C_Str() : "(node)";
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		// leave node
		if (node->mNumChildren == 0)
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		bool open = ImGui::TreeNodeEx(nodeLabel, flags);

		if (open)
		{
			for (unsigned int c = 0; c < node->mNumChildren; ++c)
			{
				DrawAiNode(model, scene, node->mChildren[c]);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}
	

	// ---- Mesh node drawing ----
	static void MeshNodeRow(const MODEL* model, const aiScene* scene, unsigned meshIdx)
	{
		if (!scene || meshIdx >= scene->mNumMeshes) return;

		aiMesh* mesh = scene->mMeshes[meshIdx];
		if (!mesh) return;

		ImGui::PushID((int)meshIdx);

		const bool selected = (g_selectedModel == model && g_selectedMesh == (int)meshIdx);

		// Visibility checkbox
		bool vis = IsMeshVisible(model, (int)meshIdx);
		if (ImGui::Checkbox("##vis", &vis))
		{
			SetMeshVisible(model, (int)meshIdx, vis);
		}
		ImGui::SameLine();

		// Icon
		Outliner::DrawIcon(Outliner::ViewKind::MESH);

		// Mesh label
		std::string meshLabel = (mesh->mName.length > 0) ?
			mesh->mName.C_Str() :
			("mesh[" + std::to_string(meshIdx) + "]");

		if (ImGui::Selectable(meshLabel.c_str(), selected))
		{
			g_selectedModel = model;
			g_selectedMesh = (int)meshIdx;
		}

		ImGui::SameLine();
		ImGui::TextDisabled(" V: %u F:%u", mesh->mNumVertices, mesh->mNumFaces);

		ImGui::PopID();
	}
	
	// Hierarcky projection for mesh node
	static void MeshNodeProjected(const MODEL* model, const aiScene* scene, const aiNode* node)
	{
		if (!scene || !node) return;

		if (!SubTreeHasMeshes(scene, node)) return;

		ImGui::PushID(node);

		const char* nodeLabel = (node->mName.length > 0) ? node->mName.C_Str() : "(node)";
		const bool hasChildren = (node->mNumChildren > 0);
		const bool hasMeshes = NodeHasMeshes(node);

		if (!hasChildren && hasMeshes) // leaf
		{
			for (unsigned int i = 0; i < node->mNumMeshes; ++i)
			{
				MeshNodeRow(model, scene, node->mMeshes[i]);
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
					MeshNodeProjected(model, scene, node->mChildren[c]);
				}

				ImGui::TreePop();
			}
		}

		ImGui::PopID();
	}

	void DrawMeshNode(const MODEL* model, const aiScene* scene)
	{
		if (!model || !scene || !scene->mRootNode) return;

		ensureState(model);
		MeshNodeProjected(model, scene, scene->mRootNode);
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
			Outliner::DrawIcon(Outliner::ViewKind::SKELETON);
			ImGui::SameLine();
			ImGui::TextUnformatted(label);
		}
		else
		{
			Outliner::DrawIcon(Outliner::ViewKind::SKELETON);
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

	void DrawSkeletonNode(const MODEL* model, const aiScene* scene)
	{
		if (!model || !scene || !scene->mRootNode) return;

		// 1. Collect all binded bones
		SkeletonUtil::BoneNameSet boneNames;
		SkeletonUtil::BuildSkeletonNameSet(scene, boneNames);
		if (boneNames.empty()) return;

		// 2. Build closure S(bones and their parents, without mRootNode)
		SkeletonUtil::NodeSet skelSet;
		SkeletonUtil::BuildSkeletonClosure(scene, boneNames, skelSet);
		if (skelSet.empty()) return;
		
		// 3. Find all skeleton tree roots
		std::vector<const aiNode*> roots;
		SkeletonUtil::FindSkeletonRoots(scene, skelSet, roots);

		// 4. Draw skeleton hierarchy
		for (const aiNode* r : roots)
		{
			DrawSkeletonSubtree(r, skelSet);
		}
	}
}
