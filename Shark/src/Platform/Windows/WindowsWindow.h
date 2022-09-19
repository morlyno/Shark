#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"

#include "Shark/Render/SwapChain.h"

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
		virtual void CreateSwapChain() override;
		virtual void ProcessEvents() const override;

		virtual inline uint32_t GetWidth() const override { return m_Size.x; }
		virtual inline uint32_t GetHeight() const override { return m_Size.y; }
		virtual const glm::uvec2& GetSize() const override { return m_Size; }
		virtual const glm::ivec2& GetPos() const override { return m_Pos; }
		virtual void ScreenToClient(glm::ivec2& screenPos) const override;

		virtual inline WindowHandle GetHandle() const override { return m_hWnd; }
		virtual Ref<SwapChain> GetSwapChain() const override { return m_SwapChain; }

		virtual inline bool IsVSync() const override { return m_VSync; }
		virtual void SetVSync(bool VSync) override { m_VSync = VSync; }
		virtual bool IsFocused() const override { return GetFocus() == m_hWnd; }

		virtual void Kill() override { DestroyWindow(m_hWnd); }
		virtual void Maximize() override { ShowWindow(m_hWnd, SW_MAXIMIZE); }

	private:
		static LRESULT WINAPI WindowProcStartUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		HWND m_hWnd;
		Ref<SwapChain> m_SwapChain;
		Ref<EventListener> m_EventListener;

		glm::uvec2 m_Size = glm::uvec2(0);
		glm::ivec2 m_Pos = glm::ivec2(0);

		std::wstring m_Name;
		bool m_VSync;

		uint16_t m_DownMouseButtons = 0;

	};

}
