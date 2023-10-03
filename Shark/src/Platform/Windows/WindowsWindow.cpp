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
#undef GetClassName

namespace Shark {

	class WindowsWindow::WindowClass : public RefCount
	{
	public:
		WindowClass::WindowClass()
			: m_HInstance(GetModuleHandleW(nullptr))
		{
			m_HInstance = GetModuleHandleA(nullptr);

			WNDCLASSEXW wc;
			ZeroMemory(&wc, sizeof(WNDCLASSEXW));
			wc.cbSize = sizeof(wc);
			wc.style = CS_OWNDC;
			wc.lpfnWndProc = &WindowsWindow::WindowProcStartUp;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = m_HInstance;
			wc.hIcon = nullptr;
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = nullptr;
			wc.lpszClassName = m_ClassName.c_str();
			wc.hIconSm = nullptr;

			RegisterClassExW(&wc);
		}

		WindowClass::~WindowClass()
		{
			UnregisterClassW(m_ClassName.c_str(), m_HInstance);
		}

		const std::wstring& GetClassName() const { return m_ClassName; }
		HINSTANCE GetHInstance() const { return m_HInstance; }

	private:
		HINSTANCE m_HInstance;
		std::wstring m_ClassName = L"Shark";
	};

	static Weak<WindowsWindow::WindowClass> s_WindowClass;

	WindowsWindow::WindowsWindow(const WindowSpecification& spec)
	{
		SK_CORE_INFO("Init Windows Window {0} {1} {2}", spec.Width, spec.Height, spec.Title);

		m_Title = spec.Title;
		m_Size = { spec.Width, spec.Height };
		m_EventListener = spec.EventListener;
		m_VSync = spec.VSync;

		m_WindowClass = s_WindowClass.TryGetRef();
		if (!m_WindowClass)
		{
			m_WindowClass = Ref<WindowClass>::Create();
			s_WindowClass = m_WindowClass;
		}

		DWORD windowFlags = WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		DWORD exWindowFlags = WS_EX_ACCEPTFILES;

		RECT windowRect{};
		windowRect.left = 100;
		windowRect.top = 100;
		windowRect.right = spec.Width + windowRect.left;
		windowRect.bottom = spec.Height + windowRect.top;
		AdjustWindowRect(&windowRect, windowFlags, false);

		std::wstring windowName = String::ToWide(m_Title);
		m_hWnd = CreateWindowExW(
			exWindowFlags,
			m_WindowClass->GetClassName().c_str(),
			windowName.c_str(),
			windowFlags,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			m_WindowClass->GetHInstance(),
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

		RAWINPUTDEVICE rawInputDevice;
		rawInputDevice.usUsagePage = 1;
		rawInputDevice.usUsage = 2;
		rawInputDevice.dwFlags = RIDEV_INPUTSINK;
		rawInputDevice.hwndTarget = m_hWnd;
		RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE));

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);

		SetFullscreen(spec.Fullscreen);

		SwapChainSpecifications swapChainSpecs;
		swapChainSpecs.Width = m_Size.x;
		swapChainSpecs.Height = m_Size.y;
		swapChainSpecs.BufferCount = 3;
		swapChainSpecs.Window = m_hWnd;
		swapChainSpecs.Fullscreen = spec.Fullscreen;
		m_SwapChain = SwapChain::Create(swapChainSpecs);
	}

	WindowsWindow::~WindowsWindow()
	{
		SK_PROFILE_FUNCTION();

		DestroyWindow(m_hWnd);
	}

	void WindowsWindow::SwapBuffers()
	{
		m_SwapChain->Present(m_VSync);
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

	void WindowsWindow::KillWindow()
	{
		DestroyWindow(m_hWnd);
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		if (m_Fullscreen == fullscreen)
			return;

		bool previousWasFullscreen = m_Fullscreen;
		m_Fullscreen = fullscreen;

		LONG windowStyle = GetWindowLong(m_hWnd, GWL_STYLE);
		const LONG fullscreenModeStyle = WS_POPUP;
		LONG windowedModeStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;

		windowedModeStyle |= WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;

		if (fullscreen)
		{
			if (previousWasFullscreen)
			{
				m_PreFullscreenWindowPlacement.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(m_hWnd, &m_PreFullscreenWindowPlacement);
			}

			windowStyle &= ~windowedModeStyle;
			windowStyle |= fullscreenModeStyle;

			SetWindowLong(m_hWnd, GWL_STYLE, windowStyle);
			SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			RECT clientRect;
			GetClientRect(m_hWnd, &clientRect);

			// Grab current monitor data for sizing
			HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &monitorInfo);

			LONG monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			//LONG targetClientWidth = std::min(monitorWidth, clientRect.right - clientRect.left);

			LONG monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
			//LONG targetClientHeight = std::min(monitorHeight, clientRect.bottom - clientRect.top);
			
			SetWindowPos(
				m_hWnd,
				NULL,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorWidth,
				monitorHeight,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING
			);

		}
		else
		{
			windowStyle &= ~fullscreenModeStyle;
			windowStyle |= windowedModeStyle;
			SetWindowLong(m_hWnd, GWL_STYLE, windowStyle);
			SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			if (m_PreFullscreenWindowPlacement.length)
			{
				SetWindowPlacement(m_hWnd, &m_PreFullscreenWindowPlacement);
			}
		}

		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);
		const int width = clientRect.right - clientRect.left;
		const int height = clientRect.bottom - clientRect.top;

		if (m_SwapChain)
		{
			m_SwapChain->SetFullscreen(fullscreen);
			// NOTE(moro): should this really be hear?
			m_SwapChain->Resize(width, height);
		}
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		m_Title = title;
		std::wstring wideTitle = String::ToWide(title);
		SetWindowTextW(m_hWnd, wideTitle.c_str());
	}

	void WindowsWindow::CenterWindow()
	{
		HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &monitorInfo);

		SetWindowPos(
			m_hWnd,
			NULL,
			(monitorInfo.rcMonitor.right - m_Size.x) / 2,
			(monitorInfo.rcMonitor.bottom - m_Size.y) / 2,
			m_Size.x,
			m_Size.y,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING
		);
	}

	glm::ivec2 WindowsWindow::ScreenToWindow(const glm::ivec2& screenPos) const
	{
		POINT p{ screenPos.x, screenPos.y };
		if (::ScreenToClient(m_hWnd, &p))
			return { p.x, p.y };
		return { 0, 0 };
	}

	glm::ivec2 WindowsWindow::WindowToScreen(const glm::ivec2& windowPos) const
	{
		POINT p{ windowPos.x, windowPos.y };
		if (::ClientToScreen(m_hWnd, &p))
			return { p.x, p.y };
		return { 0, 0 };
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_CREATE)
		{
			auto createStruct = (CREATESTRUCT*)lParam;
			auto window = (WindowsWindow*)createStruct->lpCreateParams;
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)window);
			SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)&WindowsWindow::WindowProc);
			return window->HandleMsg(hWnd, msg, wParam, lParam);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT WINAPI WindowsWindow::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto window = (WindowsWindow*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		return window->HandleMsg(hWnd, msg, wParam, lParam);
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

				// TODO(moro): figure out why the window style changes
				ShowWindow(m_hWnd, SW_SHOW);
				break;
			}

			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Pos = { pt.x, pt.y };
				m_EventListener->OnWindowMoveEvent(pt.x, pt.y);
				break;
			}

			case WM_INPUT:
			{
				RAWINPUT rawInput;
				ZeroMemory(&rawInput, sizeof(RAWINPUT));
				UINT size = sizeof(RAWINPUT);
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawInput, &size, sizeof(RAWINPUTHEADER));
				if ((rawInput.data.mouse.usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE)
				{
					const auto& mouse = rawInput.data.mouse;
					m_EventListener->OnMouseMovedRelativeEvent({ mouse.lLastX, mouse.lLastY });
				}

				if ((GET_RAWINPUT_CODE_WPARAM(wParam) & RIM_INPUT) == RIM_INPUT)
					return DefWindowProc(hWnd, msg, wParam, lParam);

				break;
			}

			case WM_NCMOUSEMOVE:
			case WM_MOUSEMOVE:
			{
				auto mousePos = ScreenToWindow({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
				m_EventListener->OnMouseMovedEvent(mousePos);
				break;
			}

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			{
				auto mousePos = ScreenToWindow({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
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
				auto mousePos = ScreenToWindow({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
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

			case WM_DROPFILES:
			{
				std::vector<std::filesystem::path> paths;

				WCHAR name[MAX_PATH];
				HDROP hDrop = (HDROP)wParam;

				UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, name, MAX_PATH);

				for (UINT i = 0; i < count; i++)
				{
					DragQueryFileW(hDrop, i, name, MAX_PATH);
					paths.emplace_back(name);
				}

				DragFinish(hDrop);

				m_EventListener->OnWindowDropEvent(std::move(paths));
				break;
			}

			default:
				return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}

}
