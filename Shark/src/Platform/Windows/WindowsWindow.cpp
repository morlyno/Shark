#include "skpch.h"
#include "WindowsWindow.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/KeyCodes.h"
#include "Shark/Core/MouseCodes.h"
#include "Shark/Render/RendererCommand.h"


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

	Scope<Window> Window::Create(const WindowProps& props)
	{
		return Create_Scope<WindowsWindow>(props.width, props.height, props.name);
	}

	WindowsWindow::WindowsWindow(int width, int height, const std::wstring& name)
		:
		m_Width(width),
		m_Height(height),
		m_Name(name),
		m_IsFocused(false),
		m_VSync(true)
	{
		std::string str;
		str.reserve(m_Name.size());
		for (auto wc : m_Name)
			str += static_cast<char>(wc);
		SK_CORE_INFO("Init Windows Window {0} {1} {2}", width, height, str);

		unsigned int flags = WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		RECT rect;
		rect.left = 100;
		rect.right = m_Width + rect.left;
		rect.top = 100;
		rect.bottom = m_Height + rect.top;
		AdjustWindowRect(&rect, flags, FALSE);

		m_Window = CreateWindowExW(
			0,
			WindowClass::GetName(),
			m_Name.c_str(),
			flags,
			600,
			180,
			rect.right - rect.left,
			rect.bottom - rect.top,
			nullptr,
			nullptr,
			WindowClass::GetHInst(),
			this
		);
		SK_ASSERT(m_Window);



		ShowWindow(m_Window, SW_SHOWDEFAULT);
		UpdateWindow(m_Window);
	}

	WindowsWindow::~WindowsWindow()
	{
		DestroyWindow(m_Window);
	}

	void WindowsWindow::Update() const
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		RendererCommand::SwapBuffer(m_VSync);
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_NCCREATE)
		{
			const CREATESTRUCTW* const pCreateStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			WindowsWindow* const pWindow = static_cast<WindowsWindow*>(pCreateStruct->lpCreateParams);
			SK_ASSERT(pWindow && "Window pointer not created");
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

	LRESULT __stdcall WindowsWindow::HandleMsg( HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam )
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
			case WM_DESTROY:
			{
				m_Callbackfunc(WindowCloseEvent(0));
				return 0;
				break;
			}

			case WM_SIZE:
			{
				m_Width = LOWORD(lParam);
				m_Height = HIWORD(lParam);
				if (m_Callbackfunc)
				{
					m_Callbackfunc(WindowResizeEvent(m_Width, m_Height));
					if (wParam == SIZE_MAXIMIZED)
					{
						m_Callbackfunc(WindowMaximizedEvent(m_Width, m_Height));
					}
					else if (wParam == SIZE_MINIMIZED)
					{
						m_Callbackfunc(WindowMinimizedEvent());
					}
				}
				break;
			}
			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				if (m_Callbackfunc)
				{
					m_Callbackfunc(WindowMoveEvent(pt.x, pt.y));
				}
				break;
			}

			// ----- Mouse ----- //
			case WM_MOUSEMOVE:
			{
				const POINTS pt = MAKEPOINTS(lParam);
				if (pt.x >= 0 && pt.x < (short)m_Width && pt.y >= 0 && pt.y < (short)m_Height)
				{
					if (!m_IsFocused)
					{
						m_Callbackfunc(WindowFocusEvent(pt.x, pt.y));
						SetCapture(m_Window);
						m_IsFocused = true;
					}
					m_Callbackfunc(MouseMoveEvent(pt.x, pt.y));
				}
				else
				{
					if (m_IsFocused)
					{
						if (wParam & (MK_LBUTTON | MK_RBUTTON))
						{
							m_Callbackfunc(MouseMoveEvent(pt.x, pt.y));
						}
						else
						{
							m_Callbackfunc(WindowLostFocusEvent());
							m_IsFocused = false;
							ReleaseCapture();
						}
					}
				}
				break;
			}
			case WM_CAPTURECHANGED:
			{
				m_IsFocused = false;
				break;
			}
			case WM_LBUTTONDOWN:
			{
				const POINTS pt = MAKEPOINTS(lParam);
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
			case WM_KEYDOWN:
			{
				const unsigned int repeat = lParam & 0xFFFF;
				m_Callbackfunc(KeyPressedEvent((KeyCode)wParam, repeat));
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
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}