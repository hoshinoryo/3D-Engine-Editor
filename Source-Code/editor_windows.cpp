/*==============================================================================

   Editor windows [editor_windows.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include "editor_windows.h"
#include "game.h"
#include "scene_manager.h"
#include "outliner.h" 
#include "default3Dmaterial.h"

#include "imgui/imgui.h"

namespace // anonymous namespace
{
	struct DefaultWindowState
	{
		const char* name;
		bool enabled;
	};

	static constexpr DefaultWindowState kDefaultWindowState[] =
	{
		{ "Outliner",         true  },
		{ "Inspector",        false },
		{ "Material Manager", false },
		{ "Attribute Editor", true  },
	};

	static bool GetDefaultEnabled(const char* windowName)
	{
		for (auto& s : kDefaultWindowState)
		{
			if (strcmp(s.name, windowName) == 0)
				return s.enabled;
		}
		return false;
	}

	// Begin window with auto-fit for first frame
	static void BeginWindowWithAutoFit(EditorUI::EditorWindow& w, const char* name)
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
		if (w.AutoFitEnabled())
		{
			flags |= ImGuiWindowFlags_AlwaysAutoResize;
		}

		ImGui::Begin(name, &w.enabled, flags);
		w.ConsumeAutoFitFrame();
	}

	template<class T>
	static void Push(std::vector<std::unique_ptr<EditorUI::EditorWindow>>& v)
	{
		auto w = std::make_unique<T>();
		w->enabled = GetDefaultEnabled(w->Name());
		v.emplace_back(std::move(w));
	}

	// Outliner window
	class OutlinerWindow final : public EditorUI::EditorWindow
	{
	public:

		const char* Name() const override { return "Outliner"; }

		void Draw(const EditorUI::Layout& l) override
		{
			ImGui::SetNextWindowPos(ImVec2(l.padding, l.padding), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(l.initWidthNarrow, l.maxHeight), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(l.minWidth, ImGui::GetTextLineHeightWithSpacing() * 15.0f),
				ImVec2(FLT_MAX, l.maxHeight)
			);

			BeginWindowWithAutoFit(*this, Name());

			ImGui::BeginChild(
				"OutlinerScroll",
				ImGui::GetContentRegionAvail(),
				false,
				ImGuiWindowFlags_AlwaysVerticalScrollbar
			);

			const auto& sceneAssets = SceneManager::AllModelAssets();

			if (ImGui::Button("Show All"))
			{
				for (auto* asset : sceneAssets)
				{
					SceneManager::SetVisibleByAsset(asset, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Hide All"))
			{
				for (auto* asset : sceneAssets)
				{
					SceneManager::SetVisibleByAsset(asset, false);
				}
			}
			ImGui::Separator();

			Outliner::ShowSceneOutliner();

			ImGui::EndChild();
			ImGui::End();
		}
	};

	// Inspector window
	class InspectorWindow final : public EditorUI::EditorWindow
	{
	public:

		const char* Name() const override { return "Inspector"; }

		void Draw(const EditorUI::Layout& l) override
		{
			const ImVec2 pos(l.displaySize.x - l.padding - l.initWidthNarrow,
				l.padding + l.initHeight + l.padding
			);

			ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(l.initWidthNarrow, l.initHeight), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(l.minWidth, ImGui::GetFrameHeightWithSpacing() * 30.0f),
				ImVec2(FLT_MAX, l.maxHeight)
			);

			BeginWindowWithAutoFit(*this, Name());

			ImGui::BeginChild(
				"InspectorScroll",
				ImGui::GetContentRegionAvail(),
				false,
				ImGuiWindowFlags_AlwaysVerticalScrollbar
			);

			ImGuiIO& io = ImGui::GetIO();

			if (ImGui::CollapsingHeader("FPS", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("FPS: %.1f", io.Framerate);
			}
			if (ImGui::CollapsingHeader("Debug Camera Control", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Game_DrawCameraDebugUI();
			}
			if (ImGui::CollapsingHeader("Debug Lighting Control", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Game_DrawLightDebugUI();
			}

			ImGui::EndChild();

			ImGui::End();
		}
	};
	
	// 
	class AttributeWindow final : public EditorUI::EditorWindow
	{
	public:

		const char* Name() const override { return "Attribute Editor"; }

		void Draw(const EditorUI::Layout& l) override
		{
			const ImVec2 posRight(l.displaySize.x - l.padding - l.initWidthNarrow, l.padding);

			ImGui::SetNextWindowPos(posRight, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(l.initWidthNarrow, l.initHeight), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(l.minWidth, ImGui::GetFrameHeightWithSpacing() * 30.0f),
				ImVec2(FLT_MAX, l.maxHeight)
			);

			BeginWindowWithAutoFit(*this, Name());

			ImGui::BeginChild(
				"AttributeEditorScroll",
				ImGui::GetContentRegionAvail(),
				false,
				ImGuiWindowFlags_AlwaysVerticalScrollbar
			);

			MeshObject* obj = SceneManager::GetSelectedObject();
			if (!obj)
			{
				ImGui::TextDisabled("No object selected.");
				//ImGui::TextDisabled(u8"(アウトライナーでオブジェクトを選択してください)");
				ImGui::EndChild();
				ImGui::End();
				return;
			}

			ImGui::Text("Name: %s", obj->name.empty() ? "(unnamed)" : obj->name.c_str());
			ImGui::Text("ID: %u", obj->id);
			ImGui::Separator();

			ImGui::Checkbox("Visible", &obj->visible);
			ImGui::SameLine();
			ImGui::Checkbox("Pickable", &obj->pickable);

			ImGui::Separator();

			// Transform
			ImGui::Text("Transform");
			ImGui::Spacing();

			TransformTRS& trs = obj->transform;

			bool changed = false;

			const float posStep = 0.05f;
			const float rotStep = 0.2f;
			const float sclStep = 0.01f;

			// position
			if (ImGui::DragFloat3("Position", &trs.position.x, posStep))
				changed = true;

			// rotation
			if (ImGui::DragFloat3("Rotation", &trs.rotationDeg.x, rotStep))
				changed = true;

			static bool s_uniformScale = false;
			ImGui::Checkbox("Uniform scale", &s_uniformScale);

			if (!s_uniformScale)
			{
				if (ImGui::DragFloat3("Scale", &trs.scale.x, sclStep))
					changed = true;
			}
			else
			{
				float u = trs.scale.x;
				if (ImGui::DragFloat("Scale", &u, sclStep))
				{
					trs.scale = { u, u, u };
					changed = true;
				}
			}

			ImGui::Spacing();
			ImGui::Separator();

			// reset buttons
			if (ImGui::Button("Reset Position"))
			{
				trs.position = { 0, 0, 0 };
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Rotation"))
			{
				trs.rotationDeg = { 0, 0, 0 };
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Scale"))
			{
				trs.scale = { 1, 1, 1 };
				changed = true;
			}

			if (changed)
			{
				obj->aabbValid = false;
			}

			ImGui::EndChild();
			ImGui::End();
		}
	};

	// Material window
	struct MaterialManagerWindow final : public EditorUI::EditorWindow
	{
		const char* Name() const override { return "Material Manager"; }

		void Draw(const EditorUI::Layout& l) override
		{
			const ImVec2 posRightDown(
				l.displaySize.x - l.padding - l.initWidthWide,
				l.displaySize.y - l.padding - l.initHeight
			);

			ImGui::SetNextWindowPos(posRightDown, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(l.initWidthWide, l.initHeight), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(l.minWidth, ImGui::GetFrameHeightWithSpacing() * 10.0f),
				ImVec2(FLT_MAX, l.maxHeight)
			);

			BeginWindowWithAutoFit(*this, Name());

			ImGui::BeginChild(
				"MaterialManagerScroll",
				ImGui::GetContentRegionAvail(),
				false,
				ImGuiWindowFlags_AlwaysVerticalScrollbar
			);

			Game_DrawMaterialManager();

			ImGui::EndChild();

			ImGui::End();
		}
	};
}

namespace EditorWindows
{
	void InitializeResources()
	{
		//Outliner::InitDefaultDrawers();
		Outliner::InitIcons(
			L"resources/mesh.png",
			L"resources/skeleton.png",
			ImVec2(16, 16)
		);
	}

	void ShutdownResources()
	{
		Outliner::ShutdownIcons();
	}

	std::vector<std::unique_ptr<EditorUI::EditorWindow>> CreateDefaultWindows()
	{
		std::vector<std::unique_ptr<EditorUI::EditorWindow>> v;

		Push<OutlinerWindow>(v);
		Push<InspectorWindow>(v);
		Push<MaterialManagerWindow>(v);
		Push<AttributeWindow>(v);

		return v;
	}
}

// ----------------------------------
// 
//マウスクリック
//  ↓
//Game_Draw()
//  ↓
//PickingPass.ReadBackId(x, y)
//  ↓
//SceneManager::SetSelectedMeshObject(id)
//  ↓
//AttributeWindow::Draw()
//→ SceneManager::GetSelectedObject()
//→ Transform / Visible / Pickable
//
//OutlinePostPass::DrawModel()
//→ SceneManager::GetSelectedObject()->id
// 
// ----------------------------------