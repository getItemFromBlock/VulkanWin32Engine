#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

#include "Maths/Maths.hpp"
#include "RenderThread.hpp"
#include "GameThread.hpp"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

WCHAR szClassName[] = L"MainClass";
WCHAR szTitle[] = L"Vulkan Demo";
HCURSOR cursorHide;
HRGN area;
UINT customMessage = 0;
std::atomic_bool captured = false;
std::atomic_bool fullscreen = false;
bool isUnitTest = false;
RenderThread rh;
GameThread gh;

struct SavedInfos
{
	RECT windowRect;
	ULONG style;
	ULONG exStyle;
	BOOL maximized;

} savedInfos = {};

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);
void OnMoveMouse(HWND hwnd, bool reset = false);
void ToggleFullscreen(HWND hwnd, bool full);
void HandleCustomMessage(HWND hWnd, WindowMessage msg, u64 payload);

std::wstring GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = GetLastError();
    if(errorMessageID == 0)
	{
        return std::wstring(); //No error message has been recorded
    }
    
    LPWSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::wstring message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}

#ifdef UNIT_TEST
int wmain(int argc, wchar_t *argv[])
{
	HINSTANCE hInstance = GetModuleHandleA(NULL);
	LPWSTR pCmdLine = GetCommandLineW();
	int nCmdShow = SW_SHOW;

	AttachConsole(ATTACH_PARENT_PROCESS);
#else
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nCmdShow)
{
#endif
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(163);
#endif
	{
		LPWSTR *arglist;
		s32 argCount = 0;
		arglist = CommandLineToArgvW(pCmdLine, &argCount);
		const std::wstring text = L"--test";
		for (s32 i = 0; i < argCount; i++)
		{
			if (text.compare(arglist[i]) == 0)
			{
				isUnitTest = true;
				break;
			}
		}
		LocalFree(arglist);

		cursorHide = nullptr;

		WNDCLASSEXW wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szClassName;
		wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

		if (!RegisterClassExW(&wcex))
		{
			MessageBoxW(NULL, L"Call to RegisterClassExW failed!", szTitle, NULL);
			return 1;
		}

		if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
		{
			MessageBoxW(NULL, L"Could not set window dpi awareness !", szTitle, NULL);
		}
		HWND hWnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, szClassName, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

		area = CreateRectRgn(0, 0, -1, -1);
		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.hRgnBlur = area;
		bb.fEnable = TRUE;
		HRESULT r = DwmEnableBlurBehindWindow(hWnd, &bb);
		if (r != S_OK)
			MessageBoxW(hWnd, L"Call to DwmEnableBlurBehindWindow failed!", szTitle, NULL);
		DeleteObject(area);

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		LONG_PTR lExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_TRANSPARENT | WS_EX_LAYERED);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, lExStyle);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		SetLayeredWindowAttributes(hWnd, RGB(255,0,0), 255, LWA_ALPHA);
		if (!hWnd)
		{
			MessageBoxW(hWnd, L"Call to CreateWindow failed!", szTitle, NULL);
			return 1;
		}

		customMessage = RegisterWindowMessageA("VulkanWin32 Custom Message");

		gh.Init(hWnd, customMessage, Maths::IVec2(800, 600), isUnitTest);
		rh.Init(hWnd, hInstance, &gh, Maths::IVec2(800, 600));

		// Main message loop:
		MSG msg;
		while (GetMessageW(&msg, NULL, 0, 0) && !rh.HasCrashed() && !gh.HasCrashed())
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		rh.Quit();
		gh.Quit();
		if (gh.HasCrashed() || rh.HasCrashed())
			return 1;
		return (int)msg.wParam;
	}
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		RECT r;
		GetClientRect(hWnd, &r);
		gh.Resize(r.right - r.left, r.bottom - r.top);
		rh.Resize(r.right - r.left, r.bottom - r.top);
		break;
	}
	case WM_CLOSE:
		rh.Quit();
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_CLEAR:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		gh.Resize(LOWORD(lParam), HIWORD(lParam));
		rh.Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 64;
		lpMMI->ptMinTrackSize.y = 128;
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		WORD keyFlags = HIWORD(lParam);
		WORD scanCode = LOBYTE(keyFlags);
		BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;
		BOOL isKeyDown = (keyFlags & KF_UP) != KF_UP;
		if (isExtendedKey)
			scanCode = MAKEWORD(scanCode, 0xE0);

		gh.SetKeyState((u8)(wParam), scanCode, isKeyDown);
		if (isKeyDown && (lParam & 0x20000000) && wParam == VK_F4)
			DestroyWindow(hWnd);
		break;
	}
	case WM_MOUSEMOVE:
		if (captured) OnMoveMouse(hWnd);
		break;
	case WM_SETCURSOR:
	{
		if (captured)
		{
			SetCursor(cursorHide);
			POINT p;
			if (GetCursorPos(&p))
			{
				SetCursorPos(p.x, p.y);
			}
		}
		else
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	default:
		if (message == customMessage)
		{
			HandleCustomMessage(hWnd, (WindowMessage)wParam, lParam);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void HandleCustomMessage(HWND hWnd, WindowMessage msg, u64 payload)
{
	switch (msg)
	{
	case NONE:
		break;
	case FULLSCREEN:
		ToggleFullscreen(hWnd, !fullscreen);
		break;
	case LOCK_MOUSE:
		captured = !captured;
		if (captured)
		{
			OnMoveMouse(hWnd, true);
			SetCursor(cursorHide);
		}
		break;
	case EXIT_WINDOW:
		PostMessageA(hWnd, WM_CLOSE, 0, 0);
		break;
	default:
		break;
	}
}

void OnMoveMouse(HWND hwnd, bool reset)
{
	RECT rect = {};
	POINT mPos = {};
	GetCursorPos(&mPos);
	GetWindowRect(hwnd, &rect);
	int tempX = rect.left + (rect.right - rect.left) / 2;
	int tempY = rect.top + (rect.bottom - rect.top) / 2;
	if (mPos.x != tempX || mPos.y != tempY) {
		int rPosX = mPos.x - tempX;
		int rPosY = mPos.y - tempY;
		if (!reset) gh.MoveMouse(Maths::Vec2(static_cast<f32>(rPosX), static_cast<f32>(rPosY)));
		SetCursorPos(tempX, tempY);
	}
}

void ToggleFullscreen(HWND hwnd, bool full)
{
	if (full == fullscreen)
		return;
	// Save current window state if not already fullscreen.
	if (!fullscreen)
	{
		// Save current window information. We force the window into restored mode
		// before going fullscreen because Windows doesn't seem to hide the
		// taskbar if the window is in the maximized state.
		savedInfos.maximized = !!IsZoomed(hwnd);
		if (savedInfos.maximized)
		SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		savedInfos.style = GetWindowLong(hwnd, GWL_STYLE);
		savedInfos.exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		GetWindowRect(hwnd, &savedInfos.windowRect);
	}

	fullscreen = full;

	if (fullscreen)
	{
		// Set new window style and size.
		SetWindowLong(	hwnd, GWL_STYLE,
						savedInfos.style & ~(WS_CAPTION | WS_THICKFRAME));
		SetWindowLong(	hwnd, GWL_EXSTYLE,
						savedInfos.exStyle & ~(WS_EX_DLGMODALFRAME |
						WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

		// On expand, if we're given a window_rect, grow to it, otherwise do
		// not resize.
		MONITORINFO monitor_info;
		monitor_info.cbSize = sizeof(monitor_info);
		GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor_info);
		RECT window_rect(monitor_info.rcMonitor);
		SetWindowPos(	hwnd, NULL, window_rect.left, window_rect.top,
						window_rect.right - window_rect.left, window_rect.bottom - window_rect.top,
						SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}
	else
	{
		// Reset original window style and size.	The multiple window size/moves
		// here are ugly, but if SetWindowPos() doesn't redraw, the taskbar won't be
		// repainted.	Better-looking methods welcome.
		SetWindowLong(hwnd, GWL_STYLE, savedInfos.style);
		SetWindowLong(hwnd, GWL_EXSTYLE, savedInfos.exStyle);

		// On restore, resize to the previous saved rect size.
		RECT new_rect(savedInfos.windowRect);
		SetWindowPos(	hwnd, NULL, new_rect.left, new_rect.top,
						new_rect.right - new_rect.left, new_rect.bottom - new_rect.top,
						SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

		if (savedInfos.maximized)
			SendMessageA(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	}
}
