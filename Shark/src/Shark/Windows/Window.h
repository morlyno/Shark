#pragma once

#include "Shark/Core.h"

#ifdef SK_PLATFORM_WINDOWS

namespace Shark {

	class SHARK_API Window
	{
	private:
		class WindowClass
		{
		private:
			WindowClass();
			~WindowClass();
			WindowClass( const WindowClass& ) = delete;
			WindowClass& operator=( const WindowClass& ) = delete;
		public:
			static const wchar_t* GetName();
			static HINSTANCE GetInstance();
		private:
			static WindowClass wndClass;
			HINSTANCE hInst;
			const wchar_t* pClassName = L"Shark\0";
		};
	public:
		Window( int width,int height,const std::wstring& name );
		~Window();
		static bool ProcessMessages();
		inline int GetWidth() const { return width; }
		inline int GetHeight() const { return height; }
	private:
		static LRESULT WINAPI WindowProcStartUp( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		static LRESULT WINAPI WindowProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		LRESULT WINAPI HandleMsg( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
	private:
		HWND hWnd;
		int width;
		int height;
	};

}

#else
#error Windows is not enabled
#endif