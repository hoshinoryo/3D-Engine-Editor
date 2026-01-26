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

namespace
{
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

	// Windows
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
		const char* Name() const override { return "Inspector"; }

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

	std::vector<std::unique_ptr<EditorUI::EditorWindow>> EditorWindows::CreateDefaultWindows()
	{
		std::vector<std::unique_ptr<EditorUI::EditorWindow>> v;

		v.emplace_back(std::make_unique<OutlinerWindow>());
		v.emplace_back(std::make_unique<InspectorWindow>());
		v.emplace_back(std::make_unique<MaterialManagerWindow>());

		return v;
	}
}


