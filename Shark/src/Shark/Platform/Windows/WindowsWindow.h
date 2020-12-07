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
		WindowsWindow( const WindowProps& props );
		~WindowsWindow();

		bool Process() const override;

		inline int GetWidth() const override { return data.width; }
		inline int GetHeight() const override { return data.height; }
		inline void* GetHandle() const override { return data.hWnd; }

		inline bool IsFocused() const override { return data.isFocused; }

		void SetEventCallbackFunc( const EventCallbackFunc& callback ) override { data.callbackfunc = callback; }

	private:
		static LRESULT WINAPI WindowProcStartUp( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		static LRESULT WINAPI WindowProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
		LRESULT WINAPI HandleMsg( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam );
	private:

		struct WindowData
		{
			unsigned int width;
			unsigned int height;
			std::wstring name;
			bool isFocused;
			EventCallbackFunc callbackfunc;
			HWND hWnd;
		};

		WindowData data;

	};

}

#else
#error Windows is not enabled
#endif