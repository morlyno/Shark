#include "skpch.h"
#include "WindowsWindow.h"
#include "Shark/Core/Log.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"

namespace Shark {

	WindowsWindow::WindowClass WindowsWindow::WindowClass::wndClass;

	WindowsWindow::WindowClass::WindowClass()
		:
		hInst( GetModuleHandle( nullptr ) )
	{
		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof( wc );
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WindowProcStartUp;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor( nullptr,IDC_ARROW );
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = ClassName;
		wc.hIconSm = nullptr;

		RegisterClassEx( &wc );
	}

	WindowsWindow::WindowClass::~WindowClass()
	{
		UnregisterClass( ClassName,hInst );
	}

	Window* Window::Create( const WindowProps& properties )
	{
		return new WindowsWindow( properties );
	}

	WindowsWindow::WindowsWindow( const WindowProps& props )
	{
		data.width = props.width;
		data.height = props.height;
		data.name = props.name;
		data.isFocused = false;

		unsigned int flags = WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		RECT rect;
		rect.left = 100;
		rect.right = data.width + rect.left;
		rect.top = 100;
		rect.bottom = data.height + rect.top;
		AdjustWindowRect( &rect,flags,FALSE );

		data.hWnd = CreateWindowEx(
			0,
			WindowClass::GetName(),
			props.name.c_str(),
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
		SK_ASSERT( data.hWnd );

		ShowWindow( data.hWnd,SW_SHOWDEFAULT );
	}

	WindowsWindow::~WindowsWindow()
	{
		DestroyWindow( data.hWnd );
	}

	bool WindowsWindow::Process() const
	{
		MSG msg = {};
		while ( PeekMessage( &msg,nullptr,0,0,PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}
		return true;
	}

	LRESULT WINAPI WindowsWindow::WindowProcStartUp( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		if ( uMsg == WM_NCCREATE )
		{
			const CREATESTRUCTW* const pCreateStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			WindowsWindow* const pWindow = static_cast<WindowsWindow*>(pCreateStruct->lpCreateParams);
			SK_ASSERT( pWindow && "Window pointer not created" );
			SetWindowLongPtr( hWnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pWindow) );
			SetWindowLongPtr( hWnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&WindowProc) );
			return pWindow->HandleMsg( hWnd,uMsg,wParam,lParam );
		}
		return DefWindowProc( hWnd,uMsg,wParam,lParam );
	}

	LRESULT WINAPI WindowsWindow::WindowProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		WindowsWindow* const pWindow = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr( hWnd,GWLP_USERDATA ));
		return pWindow->HandleMsg( hWnd,uMsg,wParam,lParam );
	}

	LRESULT __stdcall WindowsWindow::HandleMsg( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		switch ( uMsg )
		{
			case WM_DESTROY:
			{
				data.callbackfunc( WindowCloseEvent( 0 ) );
				return 0;
				break;
			}

			case WM_SIZE:
			{
				data.width = LOWORD( lParam );
				data.height = HIWORD( lParam );
				if ( data.callbackfunc )
				{
					if ( wParam == SIZE_RESTORED )
					{
						data.callbackfunc( WindowResizeEvent( data.width,data.height ) );
					}
					else if ( wParam == SIZE_MINIMIZED )
					{
						data.callbackfunc( WindowMinimizedEvent() );
					}
					else if ( wParam == SIZE_MAXIMIZED )
					{
						data.callbackfunc( WindowMaximizedEvent( data.width,data.height ) );
					}
				}
				break;
			}
			case WM_MOVE:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				if ( data.callbackfunc )
				{
					data.callbackfunc( WindowMoveEvent( pt.x,pt.y ) );
				}
				break;
			}

			// ----- Mouse ----- //
			case WM_MOUSEMOVE:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				if ( pt.x >= 0 && pt.x < (short)data.width && pt.y >= 0 && pt.y < (short)data.height )
				{
					if ( !data.isFocused )
					{
						data.callbackfunc( WindowFocusEvent( pt.x,pt.y ) );
						SetCapture( data.hWnd );
						data.isFocused = true;
					}
					data.callbackfunc( MouseMoveEvent( pt.x,pt.y ) );
				}
				else
				{
					if ( data.isFocused )
					{
						if ( wParam & (MK_LBUTTON | MK_RBUTTON) )
						{
							data.callbackfunc( MouseMoveEvent( pt.x,pt.y ) );
						}
						else
						{
							data.callbackfunc( WindowLostFocusEvent() );
							data.isFocused = false;
							ReleaseCapture();
						}
					}
				}
				break;
			}
			case WM_CAPTURECHANGED:
			{
				data.isFocused = false;
				break;
			}
			case WM_LBUTTONDOWN:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				data.callbackfunc( MouseLeftPressedEvent( pt.x,pt.y ) );
				break;
			}
			case WM_RBUTTONDOWN:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				data.callbackfunc( MouseRightPressedEvent( pt.x,pt.y ) );
				break;
			}
			case WM_LBUTTONUP:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				data.callbackfunc( MouseLeftReleasedEvent( pt.x,pt.y ) );
				break;
			}
			case WM_RBUTTONUP:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				data.callbackfunc( MouseRightReleasedEvent( pt.x,pt.y ) );
				break;
			}
			case WM_MOUSEWHEEL:
			{
				const POINTS pt = MAKEPOINTS( lParam );
				const int delta = (int)GET_WHEEL_DELTA_WPARAM( wParam );
				data.callbackfunc( MouseScrolledEvent( pt.x,pt.y,delta ) );
				break;
			}

			// ----- Keyboard ----- //
			case WM_KEYDOWN:
			{
				const unsigned int repeat = lParam & 0xFFFF;
				data.callbackfunc( KeyPressedEvent( (unsigned char)wParam,repeat ) );
				break;
			}
			case WM_KEYUP:
			{
				data.callbackfunc( KeyReleasedEvent( (unsigned char)wParam ) );
				break;
			}
			case WM_CHAR:
			{
				data.callbackfunc( KeyCharacterEvent( (unsigned char)wParam ) );
				break;
			}
		}

		return DefWindowProc( hWnd,uMsg,wParam,lParam );
	}

}