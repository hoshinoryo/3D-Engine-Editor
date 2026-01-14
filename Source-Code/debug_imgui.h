/*==============================================================================

   ImGui control [debug_imgui.h]
														 Author : Gu Anyi
														 Date   : 2025/11/02
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef DEBUG_IMGUI_H
#define DEBUG_IMGUI_H

#include <Windows.h>
#include <d3d11.h>

void Debug_Imgui_Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Debug_Imgui_Finalize();

void Debug_Imgui_Update();
void Debug_Imgui_Draw();

bool Debug_Imgui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // DEBUG_IMGUI_H