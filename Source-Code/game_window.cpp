/*==============================================================================

　 ゲームウィンドウ[game_window.cpp]
                                                         Author : Youhei Sato
                                                         Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/

#include <algorithm>

#include "game_window.h"
#include "keyboard.h"
#include "mouse.h"
#include "debug_imgui.h"
#include "imgui/imgui.h"


/*--------------------------------------------------
ウィンドウ情報
----------------------------------------------------*/
static constexpr char WINDOW_CLASS[] = "Game Window"; // メインウィンドウクラス名
static constexpr char TITLE[] = "Fake Maya"; // DO NOT TOUCH

constexpr int SCREEN_WIDTH = 1600;
constexpr int SCREEN_HEIGHT = 900;


HWND GameWindow_Create(HINSTANCE hInstance)
{
    /* ウィンドウクラスの登録 */
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION); // hInstanceが必要だから、mainで引数として渡す
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    //wcex.lpszMenuName = nullptr; // メニューは作らない
    wcex.lpszClassName = WINDOW_CLASS;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    /* メインウィンドウの作成 */

    RECT window_rect{ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

    //DWORD style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&window_rect, style, FALSE);

    const int WINDOW_WIDTH = window_rect.right - window_rect.left;
    const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top; // ウィンドウの大きさ

    // デスクトップのサイズを取得する
    // プライマリモニターの画面解像度取得
    int desktop_width = GetSystemMetrics(SM_CXSCREEN);
    int desktop_height = GetSystemMetrics(SM_CYSCREEN);

    // デスクトップの真ん中にウィンドウが生成されるように座標を計算
    // ※ただし万が一、デスクトップよりウィンドウが大きい場合は左上に表示
    const int WINDOW_X = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
    const int WINDOW_Y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

    HWND hWnd = CreateWindow(
        WINDOW_CLASS,
        TITLE,
        style,
        WINDOW_X,
        WINDOW_Y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    return hWnd;
}

/*-------------------------------------------------------------
                 ウィンドウプロシージャ
---------------------------------------------------------------*/


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (Debug_Imgui_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return 0;
    }

    ImGuiContext* ctx = ImGui::GetCurrentContext();
    bool wantMouse = false;
    bool wantKeyBoard = false;
    if (ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        wantMouse = io.WantCaptureMouse;
        wantKeyBoard = io.WantCaptureKeyboard;
    }

    switch (message)
    {
    case WM_ACTIVATEAPP:
        Keyboard_ProcessMessage(message, wParam, lParam);
        Mouse_ProcessMessage(message, wParam, lParam);
        break;
    case WM_SIZING:
    {
        const float aspect = 16.0f / 9.0f;

        RECT* rect = reinterpret_cast<RECT*>(lParam);
        int width = rect->right - rect->left;
        int height = rect->bottom - rect->top;

        const int minWidth = 1600;
        const int minHeight = 900;

        switch (wParam)
        {
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
            height = static_cast<int>(width / aspect);
            if (width < minWidth)
            {
                width = minWidth;
                height = static_cast<int>(width / aspect);
            }
            rect->bottom = rect->top + height;
            break;

        case WMSZ_TOP:
        case WMSZ_BOTTOM:
            width = static_cast<int>(height * aspect);
            if (height < minHeight)
            {
                height = minHeight;
                width = static_cast<int>(height * aspect);
            }
            rect->right = rect->left + width;
            break;

        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMRIGHT:
            width = static_cast<int>(height * aspect);
            if (width < minWidth)
            {
                width = minWidth;
                height = static_cast<int>(width / aspect);
            }
            rect->right = rect->left + width;
            rect->bottom = rect->top + height;
            break;
        }

        return TRUE;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 1600;
        mmi->ptMinTrackSize.y = 900;
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) // escapeキーを押したら終了
        {
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (!wantKeyBoard)
        {
            Keyboard_ProcessMessage(message, wParam, lParam);
        }
         return 0;
    case WM_CLOSE:
        if (MessageBox(hWnd, "終了してよろしいですか？", "終了", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
        {
            DestroyWindow(hWnd);
        }
        return 0;
    case WM_DESTROY: // ウィンドウの破棄メッセージ
        PostQuitMessage(0); // WM_QUITメッセージの送信、無効になった後にウィンドウを閉じると、プログラム全体を終了できなくなる可能性があります
        return 0;
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        if (!wantMouse)
        {
            Mouse_ProcessMessage(message, wParam, lParam);
        }
        return 0;
    default:
        // 通常のメッセージ処理はこの関数に任せる
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
