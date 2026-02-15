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
#include "default3Dshader.h"
#include "primitive_tool.h"
#include "camera_manager.h"
#include "orbit_camera.h"
#include "light.h"

#include <DirectXMath.h>

#include "imgui/imgui.h"

using namespace DirectX;

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
		{ "Primitive Tool",   true  },
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
			const ImVec2 posOutliner(l.padding, l.padding);

			ImGui::SetNextWindowPos(posOutliner, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(l.initWidthNarrow, l.maxHeight), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(l.minWidth, ImGui::GetTextLineHeightWithSpacing() * 15.0f),
				ImVec2(FLT_MAX, l.maxHeight)
			);

			BeginWindowWithAutoFit(*this, Name());

			Outliner::MenuDraw();

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
			const ImVec2 posInspector(
				l.displaySize.x - l.padding - l.initWidthNarrow,
				l.padding + l.initHeight + l.padding
			);

			ImGui::SetNextWindowPos(posInspector, ImGuiCond_FirstUseEver);
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
				if (!CameraManager::IsPlayMode())
				{
					CameraManager::GetOrbitCamera().DebugDraw();
				}
			}
			if (ImGui::CollapsingHeader("Debug Lighting Control", ImGuiTreeNodeFlags_DefaultOpen))
			{
				g_LightManager.DebugDraw();
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
			const ImVec2 posAttribute(l.displaySize.x - l.padding - l.initWidthNarrow, l.padding);

			ImGui::SetNextWindowPos(posAttribute, ImGuiCond_FirstUseEver);
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
			CubeObject* prim = PrimitiveTool::GetSelected();
			if (prim)
			{
				ImGui::Text("Type: Primitive (Cube)");
				ImGui::Text("Index: %d / Count: %d", PrimitiveTool::GetSelectedIndex(), PrimitiveTool::GetCount());
				ImGui::Separator();

				bool changed = false;

				XMFLOAT3& pos = prim->GetPositionRef();

				const float posStep = 0.05f;

				ImGui::Text("Transform");
				ImGui::Spacing();

				// position
				if (ImGui::DragFloat3("Position", &pos.x, posStep))
					changed = true;

				ImGui::Spacing();
				ImGui::Separator();

				if (ImGui::Button("Reset Position"))
				{
					pos = { 0, 0, 0 };
					changed = true;
				}

				if (changed)
				{
					prim->UpdateAABB();
				}

				ImGui::EndChild();
				ImGui::End();

				return;
			}
			else if (obj)
			{
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
				if (ImGui::DragFloat3("Rotation", &trs.rotationQuat.x, rotStep))
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
					trs.rotationQuat = { 0, 0, 0, 1 };
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

				return;
			}
			else
			{
				ImGui::TextDisabled("No object selected.");
				ImGui::EndChild();
				ImGui::End();

				return;
			}
		}
	};

	// Material window
	class MaterialManagerWindow final : public EditorUI::EditorWindow
	{
	public:

		const char* Name() const override { return "Material Manager"; }

		void Draw(const EditorUI::Layout& l) override
		{
			const ImVec2 posMaterialManager(
				l.displaySize.x - l.padding - l.initWidthWide,
				l.displaySize.y - l.padding - l.initHeight
			);

			ImGui::SetNextWindowPos(posMaterialManager, ImGuiCond_FirstUseEver);
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

			g_DefaultSceneMaterial.MenuDraw(g_Default3DshaderStatic, CameraManager::GetActiveCamera().GetPosition());

			ImGui::EndChild();

			ImGui::End();
		}
	};

	class PrimitiveToolWindow final : public EditorUI::EditorWindow
	{
	public:

		const char* Name() const override { return "Primitive Tool"; }

		void Draw(const EditorUI::Layout& l) override
		{
			const ImVec2 posPrimitiveTool(1000, 300);

			ImGui::SetNextWindowPos(posPrimitiveTool, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);

			BeginWindowWithAutoFit(*this, Name());

			PrimitiveTool::MenuDraw();

			ImGui::End();
		}
	};
}

namespace EditorWindows
{
	void InitializeResources()
	{
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
		Push<PrimitiveToolWindow>(v);

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