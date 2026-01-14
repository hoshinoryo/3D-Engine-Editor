/*==============================================================================

   Editor UI Framework [editor_ui.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/02
--------------------------------------------------------------------------------

==============================================================================*/

#include <vector>
#include <memory>

#include "imgui/imgui.h"

#include "editor_ui.h"
#include "editor_windows.h"
#include "camera_manager.h"
#include "guide_overlay.h"

namespace EditorUI
{
	// Registry
	static std::vector<std::unique_ptr<EditorWindow>> g_Windows;

	// Layout
	static Layout BuildLayout()
	{
		ImGuiIO& io = ImGui::GetIO();
		Layout l{};

		l.fontScale       = io.FontGlobalScale;
		l.displaySize     = io.DisplaySize;
		l.padding         = 40.0f;
		l.maxHeight       = io.DisplaySize.y - 20.0f;
		l.minWidth        = l.fontScale * 300.0f;
		l.initWidthNarrow = l.fontScale * 300.0f;
		l.initWidthWide   = l.fontScale * 600.0f;
		l.initHeight      = l.fontScale * 300.0f;

		return l;
	}

	void RegisterWindow(std::unique_ptr<EditorWindow> w)
	{
		g_Windows.push_back(std::move(w));
	}

	void Initialize()
	{
		if (!g_Windows.empty()) return;

		EditorWindows::InitializeResources();
		GuideOverlay::Initialize();

		auto defaults = EditorWindows::CreateDefaultWindows();

		for (auto& w : defaults)
		{
			RegisterWindow(std::move(w));
		}
	}

	void Shutdown()
	{
		g_Windows.clear();
		EditorWindows::ShutdownResources();
		GuideOverlay::Finalize();
	}

	void BeginFrame()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		ImGui::PushStyleVar(
			ImGuiStyleVar_FramePadding,
			ImVec2(style.FramePadding.x, 10.0f)
		);

		if (ImGui::BeginMainMenuBar())
		{
			// Window menu
			if (ImGui::BeginMenu("Window"))
			{
				for (auto& w : g_Windows)
				{
					ImGui::MenuItem(w->Name(), nullptr, &w->enabled);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Show All"))
				{
					for (auto& w : g_Windows) w->enabled = true;
				}
				if (ImGui::MenuItem("Hide All"))
				{
					for (auto& w : g_Windows) w->enabled = false;
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			// Play / Stop
			{
				bool isPlay = CameraManager::IsPlayMode();

				if (!isPlay)
				{
					if (ImGui::MenuItem("Play"))
					{
						CameraManager::SetPlayerMode(true);
						for (auto& w : g_Windows) w->enabled = false;
					}
				}
				else
				{
					if (ImGui::MenuItem("Stop"))
					{
						CameraManager::SetPlayerMode(false);
						for (auto& w : g_Windows) w->enabled = true;
					}
				}
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleVar();
	}

	void DrawWindows()
	{
		const Layout layout = BuildLayout();

		for (auto& w : g_Windows)
		{
			if (!w->enabled) continue;
			w->Draw(layout);
		}

		GuideOverlay::Draw();
	}
}
