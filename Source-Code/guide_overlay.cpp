/*==============================================================================

   Guide image drawing [guide_overlay.cpp]
														 Author : Gu Anyi
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#include "guide_overlay.h"
#include "texture.h"
#include "imgui/imgui.h"

// persistent flags
static bool g_AllowPlayGuide   = true;
static bool g_AllowEditorGuide = true;

// current visible flags (default)
static bool g_ShowPlayGuide   = false;
static bool g_ShowEditorGuide = false;

static Texture g_PlayGuideTex;
static Texture g_EditorGuideTex;

static void CloseCurrent();

void GuideOverlay::Initialize()
{
	g_PlayGuideTex.Load(L"resources/Guide_page_gameplay.png");
	g_EditorGuideTex.Load(L"resources/Guide_page_editor.png");

	// ƒAƒvƒŠ—§‚¿ã‚ª‚éŽž
	OnEnterEditorMode();
}

void GuideOverlay::Finalize()
{
	g_EditorGuideTex.Release();
	g_PlayGuideTex.Release();
}

void GuideOverlay::OnEnterPlayMode()
{
	if (!g_AllowPlayGuide) return;

	g_ShowPlayGuide = true;
	g_ShowEditorGuide = false;

	g_AllowPlayGuide = false;
}

void GuideOverlay::OnEnterEditorMode()
{
	if (!g_AllowEditorGuide) return;

	g_ShowEditorGuide = true;
	g_ShowPlayGuide = false;

	g_AllowEditorGuide = false;
}

void GuideOverlay::Draw()
{
	if (!g_ShowPlayGuide && !g_ShowEditorGuide) return;

	// pick current texture
	const Texture* tex = g_ShowPlayGuide ? &g_PlayGuideTex : &g_EditorGuideTex;
	if (!tex) return;

	ImTextureID imguiTex = (ImTextureID)tex->GetSRV();
	if (!imguiTex) return;

	const float w = (float)tex->GetWidth();
	const float h = (float)tex->GetHeight();

	ImGuiViewport* vp = ImGui::GetMainViewport();
	const ImVec2 vpPos = vp->WorkPos;
	const ImVec2 vpSize = vp->WorkSize;

	ImVec2 winPos(vpPos.x + (vpSize.x - w) * 0.5f, vpPos.y + (vpSize.y - h) * 0.5f);

	ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_Tooltip; // higher layer

	// An invisible window with texture drawing
	if (ImGui::Begin("##GuideOverlay", nullptr, flags))
	{
		// Window size equal to image size
		const float heightPad = 20.0f;
		ImGui::SetWindowSize(ImVec2(w, h + heightPad), ImGuiCond_Always);

		// Draw the image at the top-left of the window
		ImGui::Image(imguiTex, ImVec2(w, h));

		// Close button
		const float btnPad = 40.0f;
		ImVec2 screenPos = ImGui::GetWindowPos();
		ImVec2 btnPos(screenPos.x + w - 50.0f - btnPad, screenPos.y + btnPad);

		ImGui::SetCursorScreenPos(btnPos);
		if (ImGui::Button("Close"))
		{
			CloseCurrent();
		}
	}
	ImGui::End();
}

// Close current visibility flags
static void CloseCurrent()
{
	g_ShowPlayGuide = false;
	g_ShowEditorGuide = false;
}
