#define WIN32_LEAN_AND_MEAN
#include <SDKDDKVer.h>
#include <Windows.h>
#include <sstream>
#include <DirectXMath.h>
#include <Xinput.h>

//#include "debug_ostream.h"
#include "debug_text.h"
#include "game_window.h"
#include "direct3d.h"
#include "sampler.h"
#include "texture.h"
#include "system_timer.h"
#include "mouse.h"
#include "key_logger.h"
#include "game.h"
#include "Audio.h"
#include "collision.h"
#include "cube.h"
#include "scene.h"
#include "grid.h"
#include "light.h"
#include "debug_imgui.h"
#include "default3Dshader.h"
#include "unlit_shader.h"
#include "animation.h"
#include "demo_scene.h"

#pragma comment(lib, "xinput.lib")

using namespace DirectX;

/*--------------------------------------------------
	メイン、テンプレート
----------------------------------------------------*/


int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	(void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// DPIスケーリング
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HWND hWnd = GameWindow_Create(hInstance);

	// Initialization
	SystemTimer_Initialize();
	KeyLogger_Initialize();
	Mouse_Initialize(hWnd);
	InitAudio();

	Direct3D_Initialize(hWnd); // Direct3Dの初期化、必ず一番先頭
	
	// ---- Initialization ----
	Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Mouse_SetVisible(true);

	Animation_InitializeSkinningCB(Direct3D_GetDevice(), Direct3D_GetContext());

	g_LightManager.Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	g_Default3DshaderStatic.Initialize(Direct3D_GetDevice(), Direct3D_GetContext(), Default3DShader::Variant::Static);
	g_Default3DshaderSkinned.Initialize(Direct3D_GetDevice(), Direct3D_GetContext(), Default3DShader::Variant::Skinned);
	g_DefaultUnlitShader.Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

	Debug_Imgui_Initialize(hWnd, Direct3D_GetDevice(), Direct3D_GetContext());

	Scene_Initialize();

	Demo_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Grid_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	//Outline_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

	// Debug Mode
#if defined(DEBUG) || defined(_DEBUG)
	hal::DebugText dt(Direct3D_GetDevice(), Direct3D_GetContext(),
		L"consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(),
		Direct3D_GetBackBufferHeight(),
		0.0f, 0.0f,
		0, 0,
		0.0f, 16.0f);

	Collision_DebugInitialize(Direct3D_GetDevice(), Direct3D_GetContext());
#endif

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// fps・実行フレーム速度計測用
	double exec_last_time = SystemTimer_GetTime(); // 前回処理した時間を記録
	double fps_last_time = exec_last_time;         // fps計測開始の基準時間
	double current_time = 0.0;                     // 現在時刻（毎フレーム用）
	ULONG frame_count = 0;                         // フレーム数カウント用
	double fps = 0.0;                              // fps値を保存

	MSG msg;
	
	do
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // ゲーム処理
		{
			// fps計測処理

			current_time = SystemTimer_GetTime(); // 現在のシステム時刻を取得
			double elapsed_time = current_time - fps_last_time; // fps計測のための経過時間を計算

			if (elapsed_time >= 1.0) // 1秒ごとに計測
			{
				fps = frame_count / elapsed_time;
				fps_last_time = current_time; // 次回のためにfpsを測定した時刻保存
				frame_count = 0; // カウントをリセット
			}

			// 1/60秒ごとに実行（実行の制御）
			elapsed_time = current_time - exec_last_time; // 実行の経過時間を計算

			if (elapsed_time >= (1.0 / 60.0))
			//if (true)
			{
				exec_last_time = current_time; // 処理した時刻を保存

				// ゲームの更新
				KeyLogger_Update(); // キーの状態を更新

				//Game_Update(elapsed_time);
				Scene_Update(elapsed_time);
				
				// ゲームの描画
				Direct3D_Clear(); // Clear the screen

				Scene_Draw();

				Debug_Imgui_Update();
				Debug_Imgui_Draw();

				Direct3D_Present();
				Scene_Refresh();
			}

		}

	} while (msg.message != WM_QUIT);
	
#if defined(DEBUG) || defined(_DEBUG)
	Collision_DebugFinalize();
#endif

	Debug_Imgui_Finalize();

	g_DefaultUnlitShader.Finalize();
	g_Default3DshaderSkinned.Finalize();
	g_Default3DshaderStatic.Finalize();
	g_LightManager.Finalize();

	Animation_ReleaseSkinningCB();

	//Outline_Finalize();
	Grid_Finalize();
	Demo_Finalize();
	Scene_Finalize();
	Sampler_Finalize();

	Direct3D_Finalize();

	UninitAudio();
	Mouse_Finalize();
	
	return (int)msg.wParam;

	return 0;
}