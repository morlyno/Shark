#include "skpch.h"
#include "WindowsWindow.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseButtons.h"

#include "Shark/Utils/String.h"
#include "Shark/Render/Renderer.h"

#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/Profiler.h"

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <windowsx.h>

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
		: m_Size(props.Width, props.Height), m_Name(String::ToWideCopy(props.Name)), m_VSync(props.VSync)
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
			SK_CORE_ERROR("Error Msg: {}", WindowsUtils::TranslateErrorCode(lastErrorCode));
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

	void WindowsWindow::CreateSwapChain()
	{
		SK_PROFILE_FUNCTION();

		SwapChainSpecifications swapChainSpecs;
		swapChainSpecs.Widht = m_Size.x;
		swapChainSpecs.Height = m_Size.y;
		swapChainSpecs.BufferCount = 3;
		m_SwapChain = SwapChain::Create(swapChainSpecs);
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

		m_SwapChain->Present(m_VSync);
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

	LRESULT __stdcall WindowsWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		SK_PROFILE_FUNCTION();

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return 1;

		if (!m_Callbackfunc)
		{
			if (msg == WM_SIZE)
			{
				m_Size.x = LOWORD(lParam);
				m_Size.y = HIWORD(lParam);
			}
			else if (msg == WM_MOVE)
			{
				m_Pos.x = LOWORD(lParam);
				m_Pos.y = HIWORD(lParam);
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		switch (msg)
		{
			case WM_DESTROY:
			{
				m_Callbackfunc(WindowCloseEvent());
				return 0;
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

				const bool handled = m_Callbackfunc(WindowResizeEvent(m_Size.y, m_Size.y, state));
				Renderer::ClearAllCommandBuffers();
				m_SwapChain->Resize(m_Size.x, m_Size.y);
				return handled ? 0 : 1;
			}

			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Pos = { pt.x, pt.y };
				const bool handled = m_Callbackfunc(WindowMoveEvent(pt.x, pt.y));
				return handled ? 0 : 1;
			}

			// ----- Mouse ----- //
			case WM_NCMOUSEMOVE:
			case WM_MOUSEMOVE:
			{
				POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

				glm::ivec2 mousePos = { point.x, point.y };
				const bool handled = m_Callbackfunc(MouseMoveEvent(mousePos));

				return handled ? 0 : 1;
			}

			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			{
				POINT cursorPoint;
				cursorPoint.x = GET_X_LPARAM(lParam);
				cursorPoint.y = GET_Y_LPARAM(lParam);

				glm::ivec2 mousePos = { cursorPoint.x, cursorPoint.y };

				MouseButton::Type mouseButton = MouseButton::Invalid;
				bool doubleClick = false;
				bool mouseUp = false;

				switch (msg)
				{
				case WM_LBUTTONDOWN:
					mouseButton = MouseButton::Left;
					break;
				case WM_LBUTTONDBLCLK:
					doubleClick = true;
					mouseButton = MouseButton::Left;
					break;
				case WM_LBUTTONUP:
					mouseUp = true;
					mouseButton = MouseButton::Left;
					break;

				case WM_RBUTTONDOWN:
					mouseButton = MouseButton::Right;
					break;
				case WM_RBUTTONDBLCLK:
					doubleClick = true;
					mouseButton = MouseButton::Right;
					break;
				case WM_RBUTTONUP:
					mouseUp = true;
					mouseButton = MouseButton::Right;
					break;

				case WM_MBUTTONDOWN:
					mouseButton = MouseButton::Middle;
					break;
				case WM_MBUTTONDBLCLK:
					doubleClick = true;
					mouseButton = MouseButton::Middle;
					break;
				case WM_MBUTTONUP:
					mouseUp = true;
					mouseButton = MouseButton::Middle;
					break;

				case WM_XBUTTONDOWN:
					mouseButton = HIWORD(wParam) == 0x0001 ? MouseButton::Thumb01 : MouseButton::Thumb02;
					break;
				case WM_XBUTTONDBLCLK:
					doubleClick = true;
					mouseButton = HIWORD(wParam) == 0x0001 ? MouseButton::Thumb01 : MouseButton::Thumb02;
					break;
				case WM_XBUTTONUP:
					mouseUp = true;
					mouseButton = HIWORD(wParam) == 0x0001 ? MouseButton::Thumb01 : MouseButton::Thumb02;
					break;
				}
				
				bool handled = false;
				if (mouseUp)
				{
					handled = m_Callbackfunc(MouseButtonReleasedEvent(mousePos, mouseButton));
				}
				else if (doubleClick)
				{
					handled = m_Callbackfunc(MouseButtonDoubleClicked(mousePos, mouseButton));
				}
				else
				{
					handled = m_Callbackfunc(MouseButtonPressedEvent(mousePos, mouseButton));
				}

				return handled ? 0 : 1;
			}

			case WM_MOUSEWHEEL:
			{
				POINT mousePoint;
				mousePoint.x = GET_X_LPARAM(lParam);
				mousePoint.y = GET_Y_LPARAM(lParam);

				ScreenToClient(hWnd, &mousePoint);

				const glm::ivec2 mousePos = { mousePoint.x, mousePoint.y };
				const short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
				const float spinFactor = 1.0f / WHEEL_DELTA;

				const bool handled = m_Callbackfunc(MouseScrolledEvent(mousePos, (float)wheelDelta * spinFactor));
				return handled ? 0 : 1;
			}

			// ----- Keyboard ----- //
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				bool isRepeat = (lParam & BIT(30)) > 0;
				const bool altPressed = (lParam & BIT(29)) > 0;
				const KeyCode win32Key = (KeyCode)wParam;
				KeyCode actualKey = win32Key;

				switch (win32Key)
				{
					case VK_MENU:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftAlt;
							isRepeat = m_ExtendedKeyState[ExtendedKey::LeftAlt];
							m_ExtendedKeyState[ExtendedKey::LeftAlt] = true;
						}
						else
						{
							actualKey = Key::RightAlt;
							isRepeat = m_ExtendedKeyState[ExtendedKey::RightAlt];
							m_ExtendedKeyState[ExtendedKey::RightAlt] = true;
						}
						break;
					}
					case VK_CONTROL:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftControl;
							isRepeat = m_ExtendedKeyState[ExtendedKey::LeftControl];
							m_ExtendedKeyState[ExtendedKey::LeftControl] = true;
						}
						else
						{
							actualKey = Key::RightControl;
							isRepeat = m_ExtendedKeyState[ExtendedKey::RightControl];
							m_ExtendedKeyState[ExtendedKey::RightControl] = true;
						}
						break;
					}
					case VK_SHIFT:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftControl;
							isRepeat = m_ExtendedKeyState[ExtendedKey::LeftControl];
							m_ExtendedKeyState[ExtendedKey::LeftControl] = true;
						}
						else
						{
							actualKey = Key::RightControl;
							isRepeat = m_ExtendedKeyState[ExtendedKey::RightControl];
							m_ExtendedKeyState[ExtendedKey::RightControl] = true;
						}
						break;
					}
				}

				const bool handled = m_Callbackfunc(KeyPressedEvent(actualKey, isRepeat, altPressed));

				if (handled || msg != WM_SYSKEYDOWN)
					return 0;

				break;
			}

			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				const KeyCode win32Key = (KeyCode)wParam;
				KeyCode actualKey = win32Key;

				switch (win32Key)
				{
					case VK_MENU:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftAlt;
							m_ExtendedKeyState[ExtendedKey::LeftAlt] = false;
						}
						else
						{
							actualKey = Key::RightAlt;
							m_ExtendedKeyState[ExtendedKey::RightAlt] = false;
						}
						break;
					}
					case VK_CONTROL:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftControl;
							m_ExtendedKeyState[ExtendedKey::LeftControl] = false;
						}
						else
						{
							actualKey = Key::RightControl;
							m_ExtendedKeyState[ExtendedKey::RightControl] = false;
						}
						break;
					}
					case VK_SHIFT:
					{
						if ((lParam & BIT(24)) == 0)
						{
							actualKey = Key::LeftControl;
							m_ExtendedKeyState[ExtendedKey::LeftControl] = false;
						}
						else
						{
							actualKey = Key::RightControl;
							m_ExtendedKeyState[ExtendedKey::RightControl] = false;
						}
						break;
					}
				}

				const bool handled = m_Callbackfunc(KeyReleasedEvent((KeyCode)wParam));

				if (handled || msg == WM_SYSKEYUP)
					return 0;

				break;
			}

			case WM_CHAR:
			{
				const bool isRepeat = (lParam & BIT(30)) > 0;
				m_Callbackfunc(KeyCharacterEvent((KeyCode)wParam, isRepeat));
				return 0;
			}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}