#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"

namespace Shark {

	class WindowsWindow : public Window
	{
	private:
		class WindowClass
		{
		private:
			WindowClass();
			~WindowClass();
			WindowClass(const WindowClass&) = delete;
			WindowClass& operator=(const WindowClass&) = delete;
		public:
			static inline const wchar_t* GetName() { return wndClass.ClassName; }
			static inline HINSTANCE GetHInst() { return wndClass.hInst; }
		private:
			static WindowClass wndClass;
			HINSTANCE hInst;
			const wchar_t* ClassName = L"Shark\0";
		};
	public:
		WindowsWindow(int width, int height, const std::wstring& name);
		virtual ~WindowsWindow();

		virtual void SetEventCallbackFunc(const EventCallbackFunc& callback) override { m_Callbackfunc = callback; }

		virtual void Update() const override;

		virtual inline int GetWidth() const override { return m_Width; }
		virtual inline int GetHeight() const override { return m_Height; }
		virtual inline void* GetHandle() const override { return m_Window; }

		virtual inline bool IsFocused() const override { return m_IsFocused; }

		virtual inline bool IsVSync() const override { return m_VSync; }
		virtual void SetVSync(bool VSync) override { m_VSync = VSync; }
		
		virtual void Kill(int32_t exitcode = 0) override { DestroyWindow(m_Window); }

	private:
		static LRESULT WINAPI WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		HWND m_Window;

		uint32_t m_Width;
		uint32_t m_Height;
		std::wstring m_Name;
		bool m_IsFocused;
		bool m_VSync;

		EventCallbackFunc m_Callbackfunc;

	};

}
