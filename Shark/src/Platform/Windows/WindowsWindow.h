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
			const wchar_t* ClassName = L"Shark";
		};
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		virtual void SetEventCallbackFunc(const EventCallbackFunc& callback) override { m_Callbackfunc = callback; }

		virtual void Update() const override;

		virtual inline uint32_t GetWidth() const override { return m_Width; }
		virtual inline uint32_t GetHeight() const override { return m_Height; }
		virtual inline WindowHandle GetHandle() const override { return m_hWnd; }

		virtual inline bool IsFocused() const override { return m_IsFocused; }

		virtual inline bool IsVSync() const override { return m_VSync; }
		virtual void SetVSync(bool VSync) override { m_VSync = VSync; }
		
		virtual void Kill() override { DestroyWindow(m_hWnd); }

		virtual void Maximize() override { ShowWindow(m_hWnd, SW_MAXIMIZE); }

	private:
		static LRESULT WINAPI WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		HWND m_hWnd;

		uint32_t m_Width;
		uint32_t m_Height;
		std::wstring m_Name;
		bool m_IsFocused = false;
		bool m_IsCaptured = false;
		bool m_VSync;

		EventCallbackFunc m_Callbackfunc;

	};

}
