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
		WindowClass()
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

		~WindowClass()
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

	void WindowsWindow::ProcessEvents()
	{
		SK_PROFILE_FUNCTION();

		MSG msg = {};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (m_CursorMode == CursorMode::Locked && (m_LastCursorPosition != glm::vec2(m_Size / 2u)))
		{
			glm::vec2 center = { m_Size.x / 2, m_Size.y / 2 };
			SetCursorPositionInWindow(center);
			m_LastCursorPosition = center;
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

		::SetWindowPos(
			m_hWnd,
			NULL,
			(monitorInfo.rcMonitor.right - m_Size.x) / 2,
			(monitorInfo.rcMonitor.bottom - m_Size.y) / 2,
			m_Size.x,
			m_Size.y,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE
		);
	}

	glm::vec2 WindowsWindow::ScreenToWindow(const glm::vec2& screenPos) const
	{
		POINT p{ (LONG)screenPos.x, (LONG)screenPos.y };
		if (::ScreenToClient(m_hWnd, &p))
			return { p.x, p.y };
		return { 0, 0 };
	}

	glm::vec2 WindowsWindow::WindowToScreen(const glm::vec2& windowPos) const
	{
		POINT p{ (LONG)windowPos.x, (LONG)windowPos.y };
		if (::ClientToScreen(m_hWnd, &p))
			return { p.x, p.y };
		return { 0, 0 };
	}

	void WindowsWindow::SetCursorMode(CursorMode mode)
	{
		if (IsFocused())
		{
			if (mode == CursorMode::Locked)
			{
				m_RestoreCursorPosition = m_LastCursorPosition;
				m_VirtualCursorPosition = m_LastCursorPosition;
				SetCursorPositionInWindow({ m_Size.x / 2, m_Size.y / 2 });
			}

			if (mode == CursorMode::Locked)
				CaptureCursor();
			else
				ClipCursor(nullptr);

			if (mode != CursorMode::Locked)
				SetCursorPositionInWindow(m_RestoreCursorPosition);
		}

		if (mode == CursorMode::Normal)
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		else
			SetCursor(nullptr);

		m_CursorMode = mode;
	}

	void WindowsWindow::CaptureCursor()
	{
		RECT rect;
		rect.left = m_Pos.x;
		rect.right = m_Pos.x + m_Size.x;
		rect.top = m_Pos.y;
		rect.bottom = m_Pos.y + m_Size.y;
		ClipCursor(&rect);
	}

	void WindowsWindow::SetCursorPositionInWindow(const glm::vec2& ursorPos)
	{
		auto screenCursorPos = WindowToScreen(ursorPos);
		SetCursorPos((int)screenCursorPos.x, (int)screenCursorPos.y);
	}

	glm::vec2 WindowsWindow::GetWindowSize() const
	{
		RECT area;
		GetClientRect(m_hWnd, &area);
		return { area.right, area.bottom };
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

				bool minimized = wParam == SIZE_MINIMIZED;
				bool maximized = wParam == SIZE_MAXIMIZED;

				m_EventListener->OnWindowResizeEvent(m_Size.x, m_Size.y);
				m_EventListener->OnWindowMinimizedEvent(minimized);
				m_EventListener->OnWindowMaximizedEvent(maximized);

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

			//case WM_NCMOUSEMOVE:
			case WM_MOUSEMOVE:
			{
				const int x = GET_X_LPARAM(lParam);
				const int y = GET_Y_LPARAM(lParam);
				glm::vec2 mousePos = { x, y };

				if (m_CursorMode == CursorMode::Locked)
				{
					const auto delta = mousePos - m_LastCursorPosition;

					m_VirtualCursorPosition.x += delta.x;
					m_VirtualCursorPosition.y += delta.y;

					m_EventListener->OnMouseMovedEvent(m_VirtualCursorPosition.x, m_VirtualCursorPosition.y);
				}
				else
				{
					m_EventListener->OnMouseMovedEvent(mousePos.x, mousePos.y);
				}

				m_LastCursorPosition = mousePos;
				break;
			}

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			{
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

				m_EventListener->OnMouseButtonReleasedEvent(mouseButton);
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
					m_EventListener->OnMouseButtonDoubleClickedEvent(mouseButton);
				else
					m_EventListener->OnMouseButtonPressedEvent(mouseButton);

				break;
			}

			case WM_MOUSEWHEEL:
			{
				m_EventListener->OnMouseScrolledEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
				break;
			}
			
			case WM_MOUSEHWHEEL:
			{
				m_EventListener->OnMouseScrolledEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
				break;
			}

			// ----- Keyboard ----- //
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			{
				auto addKeyEvent = [listener = m_EventListener](KeyCode key, bool repeat, bool isDown, const ModifierKeys& modifierKeys)
				{
					if (isDown)
						listener->OnKeyPressedEvent(key, repeat, modifierKeys);
					else
						listener->OnKeyReleasedEvent(key, modifierKeys);
				};

				ModifierKeys modifierKeys;
				modifierKeys.Shift = Platform::IsKeyDown(KeyCode::Shift);
				modifierKeys.Alt = Platform::IsKeyDown(KeyCode::Alt);
				modifierKeys.Control = Platform::IsKeyDown(KeyCode::Control);

				const bool isKeyDown = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
				SK_CORE_VERIFY(wParam < 0xFF);
				int virtualKey = (int)wParam;
				KeyCode key = (KeyCode)virtualKey;
				bool repeat = lParam & BIT(30);
				bool altDown = lParam & BIT(29);
				addKeyEvent(key, repeat, isKeyDown, modifierKeys);

				if (key == KeyCode::Alt)
				{
					if (Platform::IsKeyDown(KeyCode::LeftAlt) == isKeyDown) addKeyEvent(KeyCode::LeftAlt, repeat, isKeyDown, modifierKeys);
					if (Platform::IsKeyDown(KeyCode::RightAlt) == isKeyDown) addKeyEvent(KeyCode::RightAlt, repeat, isKeyDown, modifierKeys);
				}
				else if (key == KeyCode::Control)
				{
					if (Platform::IsKeyDown(KeyCode::LeftControl) == isKeyDown) addKeyEvent(KeyCode::LeftControl, repeat, isKeyDown, modifierKeys);
					if (Platform::IsKeyDown(KeyCode::RightControl) == isKeyDown) addKeyEvent(KeyCode::RightControl, repeat, isKeyDown, modifierKeys);
				}
				else if (key == KeyCode::Shift)
				{
					if (Platform::IsKeyDown(KeyCode::LeftShift) == isKeyDown) addKeyEvent(KeyCode::LeftShift, repeat, isKeyDown, modifierKeys);
					if (Platform::IsKeyDown(KeyCode::RightShift) == isKeyDown) addKeyEvent(KeyCode::RightShift, repeat, isKeyDown, modifierKeys);
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
