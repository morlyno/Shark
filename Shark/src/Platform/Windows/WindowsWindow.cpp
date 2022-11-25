#include "skpch.h"
#include "WindowsWindow.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Input/Input.h"
#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"

#include "Shark/Utils/String.h"
#include "Shark/Render/Renderer.h"

#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Debug/Profiler.h"

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <windowsx.h>

namespace Shark {

	WindowsWindow::WindowClass WindowsWindow::WindowClass::wndClass;

	WindowsWindow::WindowClass::WindowClass()
		: hInst(GetModuleHandleW(nullptr))
	{
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(WNDCLASSEX));
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

		RegisterClassExW(&wc);
	}

	WindowsWindow::WindowClass::~WindowClass()
	{
		UnregisterClassW(ClassName, hInst);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
		: m_Size(props.Width, props.Height), m_Name(String::ToWideCopy(props.Name)), m_VSync(props.VSync), m_EventListener(props.EventListener)
	{
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
			rect.right = m_Size.x + rect.left;
			rect.top = 100;
			rect.bottom = m_Size.y + rect.top;
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
			SK_CORE_ERROR("Error Msg: {}", std::system_category().message(lastErrorCode));
			SK_CORE_ASSERT(false);
			return;
		}

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
	}

	WindowsWindow::~WindowsWindow()
	{
		DestroyWindow(m_hWnd);
	}

	void WindowsWindow::CreateSwapChain()
	{
		SwapChainSpecifications swapChainSpecs;
		swapChainSpecs.Widht = m_Size.x;
		swapChainSpecs.Height = m_Size.y;
		swapChainSpecs.BufferCount = 3;
		m_SwapChain = SwapChain::Create(swapChainSpecs);
	}

	void WindowsWindow::ProcessEvents() const
	{
		SK_PROFILE_FUNCTION();

		MSG msg = {};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	void WindowsWindow::ScreenToClient(glm::ivec2& screenPos) const
	{
		POINT p{ screenPos.x, screenPos.y };
		::ScreenToClient(m_hWnd, &p);
		screenPos.x = p.x;
		screenPos.y = p.y;
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
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
		WindowsWindow* const pWindow = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		return pWindow->HandleMsg(hWnd, uMsg, wParam, lParam);
	}

	LRESULT __stdcall WindowsWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		SK_PROFILE_FUNCTION();

		{
			SK_PROFILE_SCOPED("ImGui_ImplWin32_WndProcHandler");

			if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
				return 1;
		}

		switch (msg)
		{
			case WM_DESTROY:
			{
				m_EventListener->OnWindowCloseEvent();
				break;
			}

			case WM_SETFOCUS:
			{
				m_EventListener->OnWindowFocusEvent();
				break;
			}

			case WM_KILLFOCUS:
			{
				m_EventListener->OnWindowLostFocusEvent();
				break;
			}

			case WM_SIZE:
			{
				m_Size.x = LOWORD(lParam);
				m_Size.y = HIWORD(lParam);
				WindowResizeEvent::State state = WindowResizeEvent::State::Resize;

				if (wParam == SIZE_MAXIMIZED)
					state = WindowResizeEvent::State::Maximized;
				else if (wParam == SIZE_MINIMIZED)
					state = WindowResizeEvent::State::Minimized;

				m_EventListener->OnWindowResizeEvent(m_Size.x, m_Size.y, state);
				break;
			}

			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Pos = { pt.x, pt.y };
				m_EventListener->OnWindowMoveEvent(pt.x, pt.y);
				break;
			}

			case WM_NCMOUSEMOVE:
			case WM_MOUSEMOVE:
			{
				glm::ivec2 mousePos{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(mousePos);
				m_EventListener->OnMouseMovedEvent(mousePos);
				break;
			}

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			{
				glm::ivec2 mousePos{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(mousePos);
				MouseButton mouseButton = MouseButton::None;

				switch (msg)
				{
					case WM_LBUTTONUP:
						mouseButton = MouseButton::Left;
						break;
					case WM_RBUTTONUP:
						mouseButton = MouseButton::Right;
						break;
					case WM_MBUTTONUP:
						mouseButton = MouseButton::Middle;
						break;
					case WM_XBUTTONUP:
						mouseButton = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? MouseButton::Thumb01 : MouseButton::Thumb02;
						break;
				}

				m_DownMouseButtons &= ~(int)mouseButton;
				if (m_DownMouseButtons == 0 && ::GetCapture() == hWnd)
					::ReleaseCapture();

				m_EventListener->OnMouseButtonReleasedEvent(mousePos, mouseButton);
				break;
			}

			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONDBLCLK:
			{
				POINT cursorPoint{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				::ScreenToClient(hWnd, &cursorPoint);

				glm::ivec2 mousePos = { cursorPoint.x, cursorPoint.y };
				MouseButton mouseButton = MouseButton::None;
				bool doubleClick = false;

				switch (msg)
				{
					case WM_LBUTTONDBLCLK:
						doubleClick = true;
					case WM_LBUTTONDOWN:
						mouseButton = MouseButton::Left;
						break;

					case WM_RBUTTONDBLCLK:
						doubleClick = true;
					case WM_RBUTTONDOWN:
						mouseButton = MouseButton::Right;
						break;

					case WM_MBUTTONDBLCLK:
						doubleClick = true;
					case WM_MBUTTONDOWN:
						mouseButton = MouseButton::Middle;
						break;

					case WM_XBUTTONDBLCLK:
						doubleClick = true;
					case WM_XBUTTONDOWN:
						mouseButton = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? MouseButton::Thumb01 : MouseButton::Thumb02;
						break;
				}

				if (m_DownMouseButtons == 0 && ::GetCapture() == NULL)
					::SetCapture(hWnd);
				m_DownMouseButtons |= BIT((int)mouseButton);

				bool handled = false;
				if (doubleClick)
					m_EventListener->OnMouseButtonDoubleClickedEvent(mousePos, mouseButton);
				else
					m_EventListener->OnMouseButtonPressedEvent(mousePos, mouseButton);

				break;
			}

			case WM_MOUSEWHEEL:
			{
				glm::ivec2 mousePos{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(mousePos);
				m_EventListener->OnMouseScrolledEvent(mousePos, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
				break;
			}

			// ----- Keyboard ----- //
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			{
				auto addKeyEvent = [listener = m_EventListener](KeyCode key, bool repeat, bool isDown)
				{
					if (isDown)
						listener->OnKeyPressedEvent(key, repeat);
					else
						listener->OnKeyReleasedEvent(key);
				};

				const bool isKeyDown = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
				SK_CORE_VERIFY(wParam < 0xFF);
				int virtualKey = (int)wParam;
				KeyCode key = (KeyCode)virtualKey;
				bool repeat = lParam & BIT(30);
				bool altDown = lParam & BIT(29);
				addKeyEvent(key, repeat, isKeyDown);

				if (key == KeyCode::Alt)
				{
					if (Input::IsKeyDownAsync(KeyCode::LeftAlt) == isKeyDown) addKeyEvent(KeyCode::LeftAlt, repeat, isKeyDown);
					if (Input::IsKeyDownAsync(KeyCode::RightAlt) == isKeyDown) addKeyEvent(KeyCode::RightAlt, repeat, isKeyDown);
				}
				else if (key == KeyCode::Control)
				{
					if (Input::IsKeyDownAsync(KeyCode::LeftControl) == isKeyDown) addKeyEvent(KeyCode::LeftControl, repeat, isKeyDown);
					if (Input::IsKeyDownAsync(KeyCode::RightControl) == isKeyDown) addKeyEvent(KeyCode::RightControl, repeat, isKeyDown);
				}
				else if (key == KeyCode::Shift)
				{
					if (Input::IsKeyDownAsync(KeyCode::LeftShift) == isKeyDown) addKeyEvent(KeyCode::LeftShift, repeat, isKeyDown);
					if (Input::IsKeyDownAsync(KeyCode::RightShift) == isKeyDown) addKeyEvent(KeyCode::RightShift, repeat, isKeyDown);
				}
				break;
			}

#if 0
			case WM_CHAR:
			{
				const bool isRepeat = (lParam & BIT(30)) > 0;
				m_Callbackfunc(KeyCharacterEvent((char16_t)wParam, isRepeat));
				break;
			}
#endif

			default:
				return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}

}