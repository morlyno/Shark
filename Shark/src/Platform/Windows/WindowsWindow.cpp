#include "skpch.h"
#include "WindowsWindow.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseCodes.h"

#include "Shark/Utility/String.h"
#include "Platform/Windows/WindowsUtility.h"

#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/Profiler.h"

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Shark {

	WindowsWindow::WindowClass WindowsWindow::WindowClass::wndClass;

	WindowsWindow::WindowClass::WindowClass()
		:
		hInst(GetModuleHandle(nullptr))
	{
		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof(wc);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WindowProcStartUp;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = ClassName;
		wc.hIconSm = nullptr;

		RegisterClassEx(&wc);
	}

	WindowsWindow::WindowClass::~WindowClass()
	{
		UnregisterClass(ClassName, hInst);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
		: m_Width(props.Width), m_Height(props.Height), m_Name(String::ToWideCopy(props.Name)), m_VSync(props.VSync)
	{
		SK_PROFILE_FUNCTION();

		//std::string str;
		//str.resize(props.Name.size());
		//wcstombs(str.data(), props.Name.c_str(), (size_t)-1);
		SK_CORE_INFO("Init Windows Window {0} {1} {2}", props.Width, props.Height, props.Name);

		int width, height;
		DWORD flags = WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		
		if (props.Maximized)
		{
			flags |= WS_MAXIMIZE;
			width = CW_USEDEFAULT;
			height = CW_USEDEFAULT;
		}
		else
		{
			RECT rect;
			rect.left = 100;
			rect.right = m_Width + rect.left;
			rect.top = 100;
			rect.bottom = m_Height + rect.top;
			AdjustWindowRectEx(&rect, flags, FALSE, 0);

			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}

		m_hWnd = CreateWindowExW(
			0,
			WindowClass::GetName(),
			m_Name.c_str(),
			flags,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			nullptr,
			nullptr,
			WindowClass::GetHInst(),
			this
		);

		if (!m_hWnd)
		{
			DWORD lastErrorCode = GetLastError();
			SK_CORE_ERROR("Faled to create Window");
			SK_CORE_ERROR("Error Msg: {}", GetLastErrorMsg(lastErrorCode));
			SK_CORE_ASSERT(false);
		}

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
	}

	WindowsWindow::~WindowsWindow()
	{
		SK_PROFILE_FUNCTION();

		DestroyWindow(m_hWnd);
	}

	void WindowsWindow::Update() const
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("WindowsWindow::Update");

		MSG msg = {};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SK_PROFILE_FUNCTION();

		if (uMsg == WM_NCCREATE)
		{
			const CREATESTRUCTW* const pCreateStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			WindowsWindow* const pWindow = static_cast<WindowsWindow*>(pCreateStruct->lpCreateParams);
			SK_CORE_ASSERT(pWindow, "Window pointer not created");
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProc));
			return pWindow->HandleMsg(hWnd, uMsg, wParam, lParam);
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT WINAPI WindowsWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SK_PROFILE_FUNCTION();

		WindowsWindow* const pWindow = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		return pWindow->HandleMsg(hWnd, uMsg, wParam, lParam);
	}

	static bool g_IsRezised = false;
	static bool g_EnterSizing = false;
	static WindowResizeEvent g_LastReizeEvent = WindowResizeEvent(0, 0, WindowResizeEvent::State::Resize);

	LRESULT __stdcall WindowsWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		SK_PROFILE_FUNCTION();

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return 1;

		if (!m_Callbackfunc)
		{
			if (msg == WM_SIZE)
			{
				m_Width = LOWORD(lParam);
				m_Height = HIWORD(lParam);
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		m_IsFocused = m_hWnd == GetFocus();

		LRESULT retVal = 0;
		switch (msg)
		{
			case WM_DESTROY:
			{
				m_Callbackfunc(WindowCloseEvent());
				break;
			}

			case WM_SIZE:
			{
				g_IsRezised = true;

				m_Width = LOWORD(lParam);
				m_Height = HIWORD(lParam);
				WindowResizeEvent::State state = WindowResizeEvent::State::Resize;

				if (wParam == SIZE_MAXIMIZED)
					state = WindowResizeEvent::State::Maximized;
				else if (wParam == SIZE_MINIMIZED)
					state = WindowResizeEvent::State::Minimized;

				g_LastReizeEvent = WindowResizeEvent(m_Width, m_Height, state);
				if (!g_EnterSizing)
					m_Callbackfunc(g_LastReizeEvent);
				break;
			}

			case WM_ENTERSIZEMOVE:
			{
				g_EnterSizing = true;
				break;
			}

			case WM_EXITSIZEMOVE:
			{
				g_EnterSizing = false;
				if (g_IsRezised)
					m_Callbackfunc(g_LastReizeEvent);
				break;
			}

			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Callbackfunc(WindowMoveEvent(pt.x, pt.y));
				break;
			}

			// ----- Mouse ----- //
			case WM_MOUSEMOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				if (pt.x >= 0 && pt.x < (short)m_Width && pt.y >= 0 && pt.y < (short)m_Height)
				{
					if (!m_IsCaptured)
					{
						m_Callbackfunc(WindowFocusEvent(pt.x, pt.y));
						SetCapture(m_hWnd);
						m_IsCaptured = true;
					}
					m_Callbackfunc(MouseMoveEvent(pt.x, pt.y));
				}
				else
				{
					if (m_IsCaptured)
					{
						if (wParam & (MK_LBUTTON | MK_RBUTTON))
						{
							m_Callbackfunc(MouseMoveEvent(pt.x, pt.y));
						}
						else
						{
							m_Callbackfunc(WindowLostFocusEvent());
							m_IsCaptured = false;
							ReleaseCapture();
						}
					}
				}
				break;
			}

			case WM_CAPTURECHANGED:
			{
				m_IsCaptured = false;
				break;
			}

			case WM_LBUTTONDOWN:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				if (!m_IsFocused)
				{
					m_IsFocused = true;
					SetFocus(m_hWnd);
					m_Callbackfunc(WindowFocusEvent(pt.x, pt.y));
				}
				m_Callbackfunc(MousePressedEvent(pt.x, pt.y, Mouse::LeftButton));
				break;
			}
			case WM_RBUTTONDOWN:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Callbackfunc(MousePressedEvent(pt.x, pt.y, Mouse::RightButton));
				break;
			}
			case WM_LBUTTONUP:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Callbackfunc(MouseReleasedEvent(pt.x, pt.y, Mouse::LeftButton));
				break;
			}
			case WM_RBUTTONUP:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Callbackfunc(MouseReleasedEvent(pt.x, pt.y, Mouse::RightButton));
				break;
			}
			case WM_MOUSEWHEEL:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				const int delta = (int)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				m_Callbackfunc(MouseScrolledEvent(pt.x, pt.y, delta));
				break;
			}

			// ----- Keyboard ----- //
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				//SK_CORE_TRACE("0x{0:x}", wParam);
				const unsigned int repeat = lParam & 0xFFFF;
				const bool altPressed = (lParam & BIT(29)) > 0;
				m_Callbackfunc(KeyPressedEvent((KeyCode)wParam, repeat, altPressed));
				break;
			}

			case WM_KEYUP:
			{
				m_Callbackfunc(KeyReleasedEvent((KeyCode)wParam));
				break;
			}

			case WM_CHAR:
			{
				m_Callbackfunc(KeyCharacterEvent((KeyCode)wParam));
				break;
			}

			default: retVal = DefWindowProc(hWnd, msg, wParam, lParam);
		}

		//bool checkfocus = m_hWnd == GetFocus();
		//if (m_IsFocused != checkfocus)
		//	SK_CORE_TRACE("Focus Mismatch | {0} : {1}", m_IsFocused, checkfocus);

		//if (m_IsCaptured != (m_hWnd == GetCapture()))
		//	SK_CORE_TRACE("Capture Mismatch");

		//if (m_IsFocused != m_IsCaptured)
		//	SK_CORE_TRACE("Mismatch");

		//SK_CORE_TRACE("DefWindowProc msg: {0:x}", msg);

		return retVal;
	}

}