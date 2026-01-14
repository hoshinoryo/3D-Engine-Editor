/*==============================================================================

   ImGui control [debug_imgui.cpp]
														 Author : Gu Anyi
														 Date   : 2025/11/02
--------------------------------------------------------------------------------

==============================================================================*/

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "debug_imgui.h"
#include "editor_ui.h"

// Win32 message receive
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Debug_Imgui_Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(pDevice, pContext);

	ImGui::StyleColorsDark();

	ImGui::LoadIniSettingsFromMemory(""); // don't load from .ini file

	EditorUI::Initialize();
}

void Debug_Imgui_Finalize()
{
	EditorUI::Shutdown();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Debug_Imgui_Update()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.2f;

	// Editor frame
	EditorUI::BeginFrame();
	EditorUI::DrawWindows();
}

void Debug_Imgui_Draw()
{
	// ImGui Render and Draw
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool Debug_Imgui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}
	return false;
}
