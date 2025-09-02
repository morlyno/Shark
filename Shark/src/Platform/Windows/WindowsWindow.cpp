#include "skpch.h"
#include "WindowsWindow.h"

#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"

#include "Shark/Utils/String.h"

#include "Shark/Debug/Profiler.h"

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <windowsx.h>
#undef GetClassName
#undef IsMaximized

namespace Shark {

	enum class Style : DWORD {
		Windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		BasicBorderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
	};

	namespace utils {

		bool IsWindowMaximized(HWND hWnd)
		{
			WINDOWPLACEMENT placement;
			if (!GetWindowPlacement(hWnd, &placement))
				return false;

			return placement.showCmd == SW_MAXIMIZE;
		}

	}

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

	WindowsWindow::WindowsWindow(const WindowSpecification& specification, Ref<EventListener> listener)
		: m_Specification(specification), m_EventListener(listener)
	{
		Initialize();
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::SwapBuffers()
	{
		m_SwapChain->Present(m_Specification.VSync);
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

		const glm::vec2 halfSize = glm::vec2(GetSize() / 2u);
		if (m_CursorMode == CursorMode::Locked && (m_LastCursorPosition != halfSize))
		{
			SetCursorPositionInWindow(halfSize);
			m_LastCursorPosition = halfSize;
		}
	}

	void WindowsWindow::KillWindow()
	{
		DestroyWindow(m_WindowHandle);
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		if (m_Specification.Fullscreen == fullscreen)
			return;

		bool previousWasFullscreen = m_Specification.Fullscreen;
		m_Specification.Fullscreen = fullscreen;

		LONG windowStyle = GetWindowLong(m_WindowHandle, GWL_STYLE);
		const LONG fullscreenModeStyle = WS_POPUP;
		LONG windowedModeStyle = (LONG)Style::Windowed;

		windowedModeStyle |= WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;

		if (fullscreen)
		{
			if (previousWasFullscreen)
			{
				m_PreFullscreenWindowPlacement.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(m_WindowHandle, &m_PreFullscreenWindowPlacement);
			}

			windowStyle &= ~windowedModeStyle;
			windowStyle |= fullscreenModeStyle;

			SetWindowLong(m_WindowHandle, GWL_STYLE, windowStyle);
			SetWindowPos(m_WindowHandle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			RECT clientRect;
			GetClientRect(m_WindowHandle, &clientRect);

			// Grab current monitor data for sizing
			HMONITOR monitor = MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &monitorInfo);

			LONG monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			//LONG targetClientWidth = std::min(monitorWidth, clientRect.right - clientRect.left);

			LONG monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
			//LONG targetClientHeight = std::min(monitorHeight, clientRect.bottom - clientRect.top);
			
			SetWindowPos(
				m_WindowHandle,
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
			SetWindowLong(m_WindowHandle, GWL_STYLE, windowStyle);
			SetWindowPos(m_WindowHandle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			if (m_PreFullscreenWindowPlacement.length)
			{
				SetWindowPlacement(m_WindowHandle, &m_PreFullscreenWindowPlacement);
			}
		}

		RECT clientRect;
		GetClientRect(m_WindowHandle, &clientRect);
		const int width = clientRect.right - clientRect.left;
		const int height = clientRect.bottom - clientRect.top;

		if (m_SwapChain)
		{
#if TODO // #Renderer #Disabled
			m_SwapChain->SetFullscreen(fullscreen);
#endif
			// NOTE(moro): should this really be hear?
			m_SwapChain->Resize(width, height);
		}
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		m_Specification.Title = title;
		std::wstring wideTitle = String::ToWide(title);
		SetWindowTextW(m_WindowHandle, wideTitle.c_str());
	}

	void WindowsWindow::Restore()
	{
		ShowWindow(m_WindowHandle, SW_RESTORE);
	}

	void WindowsWindow::Minimize()
	{
		ShowWindow(m_WindowHandle, SW_MINIMIZE);
	}

	void WindowsWindow::Maximize()
	{
		ShowWindow(m_WindowHandle, SW_MAXIMIZE);
	}

	void WindowsWindow::CenterWindow()
	{
		HMONITOR monitor = MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &monitorInfo);

		::SetWindowPos(
			m_WindowHandle,
			NULL,
			(monitorInfo.rcMonitor.right - m_Specification.Width) / 2,
			(monitorInfo.rcMonitor.bottom - m_Specification.Height) / 2,
			m_Specification.Width,
			m_Specification.Height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOSIZE
		);
	}

	glm::vec2 WindowsWindow::ScreenToWindow(const glm::vec2& screenPos) const
	{
		POINT p{ (LONG)screenPos.x, (LONG)screenPos.y };
		if (::ScreenToClient(m_WindowHandle, &p))
			return { p.x, p.y };
		return { 0, 0 };
	}

	glm::vec2 WindowsWindow::WindowToScreen(const glm::vec2& windowPos) const
	{
		POINT p{ (LONG)windowPos.x, (LONG)windowPos.y };
		if (::ClientToScreen(m_WindowHandle, &p))
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
				SetCursorPositionInWindow({ m_Specification.Width / 2, m_Specification.Height / 2 });
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
		rect.left = m_Position.x;
		rect.right = m_Position.x + m_Specification.Width;
		rect.top = m_Position.y;
		rect.bottom = m_Position.y + m_Specification.Height;
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
		GetClientRect(m_WindowHandle, &area);
		return { area.right, area.bottom };
	}

	void WindowsWindow::Initialize()
	{
		SK_CORE_WARN_TAG("Window", "Init Windows Window {0} {1} {2}", m_Specification.Width, m_Specification.Height, m_Specification.Title);

		m_WindowClass = s_WindowClass.TryGetRef();
		if (!m_WindowClass)
		{
			m_WindowClass = Ref<WindowClass>::Create();
			s_WindowClass = m_WindowClass;
		}

		if (!CreateNativeWindow())
			return;

		RAWINPUTDEVICE rawInputDevice;
		rawInputDevice.usUsagePage = 1;
		rawInputDevice.usUsage = 2;
		rawInputDevice.dwFlags = RIDEV_INPUTSINK;
		rawInputDevice.hwndTarget = m_WindowHandle;
		RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE));

		ShowWindow(m_WindowHandle, SW_SHOW);
		UpdateWindow(m_WindowHandle);

		SetFullscreen(m_Specification.Fullscreen);

		SwapChainSpecification swapChainSpecs;
		swapChainSpecs.Width = m_Specification.Width;
		swapChainSpecs.Height = m_Specification.Height;
		swapChainSpecs.Window = m_WindowHandle;
		swapChainSpecs.Fullscreen = m_Specification.Fullscreen;
		m_SwapChain = SwapChain::Create(swapChainSpecs);
	}

	void WindowsWindow::Shutdown()
	{
		SK_PROFILE_FUNCTION();

		m_SwapChain = nullptr;
		DestroyWindow(m_WindowHandle);
		m_WindowHandle = NULL;
	}

	bool WindowsWindow::CreateNativeWindow()
{
		DWORD windowFlags = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		{
			windowFlags |= WS_SYSMENU | WS_MINIMIZEBOX;
			if (m_Specification.Decorated)
			{
				windowFlags |= WS_CAPTION;

				// if resizable
				windowFlags |= WS_MAXIMIZEBOX | WS_THICKFRAME;
			}
			else
			{
				windowFlags |= WS_POPUP;
			}
		}

		DWORD exWindowFlags = WS_EX_ACCEPTFILES | WS_EX_APPWINDOW;

		RECT windowRect{};
		windowRect.left = 100;
		windowRect.top = 100;
		windowRect.right = m_Specification.Width + windowRect.left;
		windowRect.bottom = m_Specification.Height + windowRect.top;
		AdjustWindowRectEx(&windowRect, windowFlags, false, exWindowFlags);

		std::wstring windowName = String::ToWide(m_Specification.Title);
		m_WindowHandle = CreateWindowExW(exWindowFlags,
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
										 this);

		if (!m_WindowHandle)
		{
			DWORD lastErrorCode = GetLastError();
			SK_CORE_ERROR_TAG("Window", "Faled to create Window");
			SK_CORE_ERROR_TAG("Window", "Error Msg: {}", std::system_category().message(lastErrorCode));
			SK_CORE_ASSERT(false);
			return false;
		}

		{
			RECT rect = { 0, 0, m_Specification.Width, m_Specification.Height };
			WINDOWPLACEMENT windowPlacement = {};
			windowPlacement.length = sizeof(WINDOWPLACEMENT);

			ClientToScreen(m_WindowHandle, (POINT*)&rect.left);
			ClientToScreen(m_WindowHandle, (POINT*)&rect.right);

			AdjustWindowRectEx(&rect, windowFlags, FALSE, exWindowFlags);

			// Only update the restored window rect as the window may be maximized
			GetWindowPlacement(m_WindowHandle, &windowPlacement);
			windowPlacement.rcNormalPosition = rect;
			windowPlacement.showCmd = SW_HIDE;
			SetWindowPlacement(m_WindowHandle, &windowPlacement);
		}

		return true;
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_CREATE)
		{
			auto createStruct = (CREATESTRUCT*)lParam;
			auto window = (WindowsWindow*)createStruct->lpCreateParams;
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)window);
			SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)&WindowsWindow::WindowProc);

			const BOOL hasThickFrame = GetWindowLongPtr(hWnd, GWL_STYLE) & WS_THICKFRAME;
			if (hasThickFrame)
			{
				RECT size_rect;
				GetWindowRect(hWnd, &size_rect);

				// Inform the application of the frame change to force redrawing with the new
				// client area that is extended into the title bar
				SetWindowPos(
					hWnd, NULL,
					size_rect.left, size_rect.top,
					size_rect.right - size_rect.left, size_rect.bottom - size_rect.top,
					SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE
				);
			}

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

		static RECT border_thickness = { 4, 4, 4, 4 };
		BOOL hasThickFrame = GetWindowLongPtr(hWnd, GWL_STYLE) & WS_THICKFRAME;

		switch (msg)
		{
			case WM_DESTROY:
			{
				m_EventListener->OnWindowCloseEvent();
				return 0;
			}

			case WM_SETFOCUS:
			{
				m_EventListener->OnWindowFocusEvent();
				return 0;
			}

			case WM_KILLFOCUS:
			{
				m_EventListener->OnWindowLostFocusEvent();
				return 0;
			}

			case WM_SIZE:
			{
				m_Specification.Width = LOWORD(lParam);
				m_Specification.Height = HIWORD(lParam);

				bool minimized = wParam == SIZE_MINIMIZED;
				bool maximized = wParam == SIZE_MAXIMIZED;

				m_EventListener->OnWindowResizeEvent(m_Specification.Width, m_Specification.Height);
				if (m_IsMinimized != minimized)
					m_EventListener->OnWindowMinimizedEvent(minimized);
				if (m_IsMaximized != maximized)
					m_EventListener->OnWindowMaximizedEvent(maximized);

				m_IsMinimized = minimized;
				m_IsMaximized = maximized;

				RECT sizeRect;
				GetWindowRect(m_WindowHandle, &sizeRect);

				SetWindowPos(m_WindowHandle, nullptr,
							 sizeRect.left, sizeRect.top,
							 sizeRect.right - sizeRect.left,
							 sizeRect.bottom - sizeRect.top,
							 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

				return 0;
			}

			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				m_Position = { pt.x, pt.y };
				m_EventListener->OnWindowMoveEvent(pt.x, pt.y);
				return 0;
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
				return 0;
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
				return 0;
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

				return 0;
			}

			case WM_MOUSEWHEEL:
			{
				m_EventListener->OnMouseScrolledEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
				return 0;
			}
			
			case WM_MOUSEHWHEEL:
			{
				m_EventListener->OnMouseScrolledEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
				return 0;
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
				SK_CORE_ASSERT(wParam < 0xFF);
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
				return 0;
			}

#if 0
			case WM_CHAR:
			{
				const bool isRepeat = (lParam & BIT(30)) > 0;
				m_Callbackfunc(KeyCharacterEvent((char16_t)wParam, isRepeat));
				break;
			}
#endif

			case WM_SYSCHAR:
			{
				return 0;
			}

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
				return 0;
			}

			case WM_SETCURSOR:
			{
				if (m_CursorMode != CursorMode::Normal)
					return 0;

				WORD hit = LOWORD(lParam);

				if ((HWND)wParam == m_WindowHandle)
				{
					switch (hit)
					{
						case HTLEFT:
						case HTRIGHT:
							SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
							return 1;

						case HTTOP:
						case HTBOTTOM:
							SetCursor(LoadCursor(nullptr, IDC_SIZENS));
							return 1;

						case HTTOPLEFT:
						case HTBOTTOMRIGHT:
							SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
							return 1;

						case HTTOPRIGHT:
						case HTBOTTOMLEFT:
							SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
							return 1;
					}
				}

				SetCursor(LoadCursor(nullptr, IDC_ARROW));
				return 1;
			}

			case WM_ACTIVATE:
			{
				if (!m_Specification.CustomTitlebar)
					break;

				RECT title_bar_rect = { 0 };
				InvalidateRect(hWnd, &title_bar_rect, FALSE);
				break;
			}

			case WM_NCCALCSIZE:
			{
				if (!m_Specification.CustomTitlebar || !hasThickFrame || !wParam)
					break;


				// Shrink client area by border thickness so we can
				// resize window and see borders
				const int resizeBorderX = GetSystemMetrics(SM_CXFRAME);
				const int resizeBorderY = GetSystemMetrics(SM_CYFRAME);

				NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
				RECT* requestedClientRect = params->rgrc;

				if (IsMaximized())
				{
					HMONITOR monitorHandle = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
					MONITORINFO monitorInfo = {};
					monitorInfo.cbSize = sizeof(MONITORINFO);
					GetMonitorInfo(monitorHandle, &monitorInfo);
					requestedClientRect->left = monitorInfo.rcWork.left;
					requestedClientRect->right = monitorInfo.rcWork.right;
					requestedClientRect->top = monitorInfo.rcWork.top;
					requestedClientRect->bottom = monitorInfo.rcWork.bottom;
				}
				else
				{
					requestedClientRect->left += resizeBorderX;
					requestedClientRect->right -= resizeBorderX;
					requestedClientRect->bottom -= resizeBorderY;
				}

				// NOTE(moro): doesn't work on windows 10
				//requestedClientRect->top += resizeBorderY;

				return WVR_ALIGNTOP | WVR_ALIGNLEFT;
			}

			case WM_NCHITTEST:
			{
				if (!m_Specification.CustomTitlebar || !hasThickFrame)
					break;

				//
				// Hit test for custom frames
				//
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(hWnd, &pt);

				// Check borders first
				if (!m_IsMaximized)
				{
					RECT rc;
					GetClientRect(hWnd, &rc);

					const int verticalBorderSize = GetSystemMetrics(SM_CYFRAME);

					enum { left = 1, top = 2, right = 4, bottom = 8 };
					int hit = 0;
					if (pt.x <= border_thickness.left)
						hit |= left;
					if (pt.x >= rc.right - border_thickness.right)
						hit |= right;
					if (pt.y <= border_thickness.top || pt.y < verticalBorderSize)
						hit |= top;
					if (pt.y >= rc.bottom - border_thickness.bottom)
						hit |= bottom;

					if (hit & top && hit & left)        return HTTOPLEFT;
					if (hit & top && hit & right)       return HTTOPRIGHT;
					if (hit & bottom && hit & left)     return HTBOTTOMLEFT;
					if (hit & bottom && hit & right)    return HTBOTTOMRIGHT;
					if (hit & left)                     return HTLEFT;
					if (hit & top)                      return HTTOP;
					if (hit & right)                    return HTRIGHT;
					if (hit & bottom)                   return HTBOTTOM;
				}

				if (m_TitlebarHitTestCallback)
				{
					bool titlebarHittest = false;
					m_TitlebarHitTestCallback(pt.x, pt.y, titlebarHittest);
					if (titlebarHittest)
						return HTCAPTION;
				}

				// In client area
				return HTCLIENT;
			}

		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}
