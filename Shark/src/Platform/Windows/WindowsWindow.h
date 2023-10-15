#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"

#include "Shark/Render/SwapChain.h"

namespace Shark {

	enum class CursorMode;

	class WindowsWindow : public Window
	{
	public:
		class WindowClass;

	public:
		WindowsWindow(const WindowSpecification& spec);
		virtual ~WindowsWindow();
		
		virtual void SwapBuffers() override;
		virtual void ProcessEvents() override;

		virtual void KillWindow() override;

		virtual void SetFullscreen(bool fullscreen) override;
		virtual bool IsFullscreen() const override { return m_Fullscreen; }

		virtual void SetTitle(const std::string& title) override;
		virtual const std::string& GetTitle() const override { return m_Title; }

		virtual void Maximize() override { ShowWindow(m_hWnd, SW_MAXIMIZE); }
		virtual void CenterWindow() override;
		virtual bool IsResizable() const override { return true; }
		virtual void SetResizable(bool resizable) override {}

		virtual bool IsFocused() const override { return GetFocus() == m_hWnd; }
		virtual void SetFocused() override { SetFocus(m_hWnd); }

		virtual bool VSyncEnabled() const override { return m_VSync; }
		virtual void EnableVSync(bool enabled) override { m_VSync = enabled; }

		virtual uint32_t GetWidth() const override { return m_Size.x; }
		virtual uint32_t GetHeight() const override { return m_Size.y; }
		virtual const glm::uvec2& GetSize() const override { return m_Size; }
		virtual const glm::ivec2& GetPosition() const override { return m_Pos; }
		virtual glm::vec2 ScreenToWindow(const glm::vec2& screenPos) const override;
		virtual glm::vec2 WindowToScreen(const glm::vec2& windowPos) const override;

		virtual void SetCursorMode(CursorMode mode) override;

		virtual WindowHandle GetHandle() const override { return m_hWnd; }
		virtual Ref<SwapChain> GetSwapChain() const override { return m_SwapChain; }

	public:
		void CaptureCursor();
		void SetCursorPositionInWindow(const glm::vec2& cursorPos);
		glm::vec2 GetWindowSize() const;

	private:
		static LRESULT WINAPI WindowProcStartUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		Ref<WindowClass> m_WindowClass;

		HWND m_hWnd;
		Ref<SwapChain> m_SwapChain;
		Ref<EventListener> m_EventListener;

		glm::uvec2 m_Size = glm::uvec2(0);
		glm::ivec2 m_Pos = glm::ivec2(0);

		std::string m_Title;
		bool m_VSync;

		uint16_t m_DownMouseButtons = 0;

		bool m_Fullscreen = false;
		WINDOWPLACEMENT m_PreFullscreenWindowPlacement{};

		CursorMode m_CursorMode = CursorMode::Normal;
		glm::vec2 m_RestoreCursorPosition;

		glm::vec2 m_LastCursorPosition;
		glm::vec2 m_VirtualCursorPosition;
	};

}
