#include "Engine.h"
#include "App.h"
#include <Windows.h>
#include <windowsx.h>

using namespace moonriver;

static bool g_mouse_down = false;
static bool g_minimized = false;
static int g_window_width;
static int g_window_height;
static Engine* g_engine;
static App* g_App;

static void SwitchFullScreen(HWND hWnd)
{
    static bool full_screen = false;
    static int old_style = 0;
    static RECT old_pos;

    if (!full_screen)
    {
        full_screen = true;

        old_style = GetWindowLong(hWnd, GWL_STYLE);
        GetWindowRect(hWnd, &old_pos);

        RECT rect;
        HWND desktop = GetDesktopWindow();
        GetWindowRect(desktop, &rect);
        SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPED);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, rect.right, rect.bottom, SWP_SHOWWINDOW);
    }
    else
    {
        full_screen = false;

        SetWindowLong(hWnd, GWL_STYLE, old_style);
        SetWindowPos(hWnd, HWND_NOTOPMOST,
            old_pos.left,
            old_pos.top,
            old_pos.right - old_pos.left,
            old_pos.bottom - old_pos.top,
            SWP_SHOWWINDOW);
    }
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            delete g_App;
            Engine::Destroy(&g_engine);
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
            {
                g_minimized = true;
            }
            else
            {
                if (!g_minimized)
                {
                    int width = lParam & 0xffff;
                    int height = (lParam & 0xffff0000) >> 16;

                    g_window_width = width;
                    g_window_height = height;
                }

                g_minimized = false;
            }
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {

            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {

            break;
        }

        case WM_CHAR:
            if (wParam > 0 && wParam < 0x10000)
            {
                unsigned short c = (unsigned short) wParam;
            }
            break;

        case WM_SYSCHAR:
            if (wParam == VK_RETURN)
            {
                // Alt + Enter
                SwitchFullScreen(hWnd);
            }
            break;

        case WM_LBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_RBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_MBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);


            break;
        }

        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_LBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_RBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_MBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            break;
        }

        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            break;
        }

        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int CreateAppWindow()
{
#if defined(_DEBUG) || defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    const char* name = "MrEngine";
    int window_width = 1280;
    int window_height = 720;

    WNDCLASSEX win_class;
    ZeroMemory(&win_class, sizeof(win_class));

    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WindowProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = nullptr;
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = nullptr;
    win_class.lpszClassName = name;
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.hIcon = (HICON)LoadImage(nullptr, "icon.ico", IMAGE_ICON, SM_CXICON, SM_CYICON, LR_LOADFROMFILE);
    win_class.hIconSm = win_class.hIcon;

    if (!RegisterClassEx(&win_class))
    {
        return 0;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD style_ex = 0;

    RECT wr = { 0, 0, window_width, window_height };
    AdjustWindowRect(&wr, style, FALSE);

    HWND hwnd = nullptr;

    {
        int x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2 + wr.left;
        int y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2 + wr.top;
        int w = wr.right - wr.left;
        int h = wr.bottom - wr.top;

        hwnd = CreateWindowEx(
            style_ex,			// window ex style
            name,		        // class name
            name,		        // app name
            style,			    // window style
            x, y,				// x, y
            w, h,               // w, h
            nullptr,		    // handle to parent
            nullptr,            // handle to menu
            nullptr,			// hInstance
            nullptr);           // no extra parameters
    }

    if (!hwnd)
    {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    g_window_width = window_width;
    g_window_height = window_height;

    g_engine = Engine::Create(hwnd, g_window_width, g_window_height);
    g_App = new App(g_engine);

    bool exit = false;
    MSG msg;

    while (true)
    {
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                exit = true;
                break;
            }
            else
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }

        if (exit)
        {
            break;
        }

        if (g_minimized)
        {
            continue;
        }

        if (g_engine)
        {
            if (g_window_width != g_engine->GetWidth() || g_window_height != g_engine->GetHeight())
            {
                g_engine->OnResize(hwnd, g_window_width, g_window_height);
            }

            g_App->Update();
            g_engine->Execute();

            if (g_engine->HasQuit())
            {
                SendMessageA(hwnd, WM_CLOSE, 0, 0);
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    CreateAppWindow();
    return 0;
}
