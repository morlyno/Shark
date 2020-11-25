#include "skpch.h"
#include "Window.h"
#include "Shark/Log.h"

namespace Shark {

	Window::WindowClass Window::WindowClass::wndClass;

	Window::WindowClass::WindowClass()
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
		wc.lpszClassName = pClassName;
		wc.hIconSm = nullptr;

		RegisterClassEx( &wc );
	}

	Window::WindowClass::~WindowClass()
	{
		UnregisterClass( pClassName,hInst );
	}

	const wchar_t* Window::WindowClass::GetName()
	{
		return wndClass.pClassName;
	}

	HINSTANCE Window::WindowClass::GetInstance()
	{
		return wndClass.hInst;
	}

	Window::Window( int width,int height,const std::wstring& name )
		:
		width( width ),
		height( height )
	{
		unsigned int flags = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		RECT rect;
		rect.left = 100;
		rect.right = width + rect.left;
		rect.top = 100;
		rect.bottom = height + rect.top;
		AdjustWindowRect( &rect,flags,FALSE );

		hWnd = CreateWindowEx(
			0,
			WindowClass::GetName(),
			name.c_str(),
			flags,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rect.right - rect.left,
			rect.bottom - rect.top,
			nullptr,
			nullptr,
			WindowClass::GetInstance(),
			this
		);
		SK_ASSERT( hWnd );

		ShowWindow( hWnd,SW_SHOWDEFAULT );
	}

	Window::~Window()
	{
		DestroyWindow( hWnd );
	}

	bool Window::ProcessMessages()
	{
		MSG msg = {};
		while ( PeekMessage( &msg,nullptr,0,0,PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				return false;
			}
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}
		return true;
	}

	LRESULT WINAPI Window::WindowProcStartUp( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		if ( uMsg == WM_NCCREATE )
		{
			const CREATESTRUCTW* const pCreateStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* const pWindow = static_cast<Window*>(pCreateStruct->lpCreateParams);
			SK_ASSERT( pWindow && "Window pointer not created" );
			SetWindowLongPtr( hWnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pWindow) );
			SetWindowLongPtr( hWnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&WindowProc) );
			return pWindow->HandleMsg( hWnd,uMsg,wParam,lParam );
		}
		return DefWindowProc( hWnd,uMsg,wParam,lParam );
	}

	LRESULT WINAPI Window::WindowProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtr( hWnd,GWLP_USERDATA ));
		return pWindow->HandleMsg( hWnd,uMsg,wParam,lParam );
	}

	LRESULT __stdcall Window::HandleMsg( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
	{
		switch ( uMsg )
		{
			case WM_DESTROY:
				PostQuitMessage( 0 );
				return 0;
		}

		return DefWindowProc( hWnd,uMsg,wParam,lParam );
	}

}