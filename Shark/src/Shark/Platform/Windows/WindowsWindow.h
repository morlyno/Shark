#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Core/Window.h"

#ifdef SK_PLATFORM_WINDOWS

namespace Shark {

	class WindowsWindow : public Window
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
			static inline const wchar_t* GetName() { return wndClass.ClassName; }
			static inline HINSTANCE GetHInst() { return wndClass.hInst; }
		private:
			static WindowClass wndClass;
			HINSTANCE hInst;
			const wchar_t* ClassName = L"Shark\0";
		};
	public:
		WindowsWindow( int width,int height,const std::wstring& name );
		~WindowsWindow();

		bool Process() const override;

		inline int GetWidth() const override { return m_Width; }
		inline int GetHeight() const override { return m_Height; }
		inline void* GetHandle() const override { return m_Window; }

		inline bool IsFocused() const override { return m_IsFocused; }

		void SetEventCallbackFunc( const EventCallbackFunc& callback ) override { m_Callbackfunc = callback; }

	private:
		static LRESULT WINAPI WindowProcStartUp( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		static LRESULT WINAPI WindowProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		LRESULT WINAPI HandleMsg( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
	private:
		HWND m_Window;
		unsigned int m_Width;
		unsigned int m_Height;
		std::wstring m_Name;
		bool m_IsFocused;

		EventCallbackFunc m_Callbackfunc;

	};

}

#else
#error Windows is not enabled
#endif