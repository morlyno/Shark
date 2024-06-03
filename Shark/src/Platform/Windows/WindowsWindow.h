#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Window.h"

#include "Shark/Render/SwapChain.h"

#undef IsMaximized

namespace Shark {

	enum class CursorMode;

	class WindowsWindow : public Window
	{
	public:
		class WindowClass;

	public:
		WindowsWindow(const WindowSpecification& specification, Ref<EventListener> listener);
		virtual ~WindowsWindow();

		virtual void SetTitlebarHitTestCallback(const std::function<void(int, int, bool&)>& callback) override { m_TitlebarHitTestCallback = callback; }

		virtual void SwapBuffers() override;
		virtual void ProcessEvents() override;

		virtual void KillWindow() override;

		virtual void SetFullscreen(bool fullscreen) override;
		virtual bool IsFullscreen() const override { return m_Specification.Fullscreen; }

		virtual void SetTitle(const std::string& title) override;
		virtual const std::string& GetTitle() const override { return m_Specification.Title; }

		virtual void Restore() override;
		virtual void Minimize() override;
		virtual void Maximize() override;

		virtual void CenterWindow() override;
		virtual bool IsResizable() const override { return true; }
		virtual void SetResizable(bool resizable) override {}

		virtual bool IsFocused() const override { return GetFocus() == m_WindowHandle; }
		virtual void SetFocused() override { SetFocus(m_WindowHandle); }

		virtual bool IsMinimized() const override { return m_IsMinimized; }
		virtual bool IsMaximized() const override { return m_IsMaximized; }

		virtual bool VSyncEnabled() const override { return m_Specification.VSync; }
		virtual void EnableVSync(bool enabled) override { m_Specification.VSync = enabled; }

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual glm::uvec2 GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }
		virtual glm::ivec2 GetPosition() const override { return m_Position; }
		virtual glm::vec2 ScreenToWindow(const glm::vec2& screenPos) const override;
		virtual glm::vec2 WindowToScreen(const glm::vec2& windowPos) const override;

		virtual void SetCursorMode(CursorMode mode) override;

		virtual WindowHandle GetHandle() const override { return m_WindowHandle; }
		virtual Ref<SwapChain> GetSwapChain() const override { return m_SwapChain; }

	public:
		void CaptureCursor();
		void SetCursorPositionInWindow(const glm::vec2& cursorPos);
		glm::vec2 GetWindowSize() const;

	private:
		void Initialize();
		void Shutdown();

		bool CreateNativeWindow();

		DWORD GetWindowStyle() const;
		DWORD GetWindowExStyle() const;

	private:
		static LRESULT WINAPI WindowProcStartUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT WINAPI WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		Ref<WindowClass> m_WindowClass;
		WindowSpecification m_Specification;

		HWND m_WindowHandle = NULL;
		Ref<SwapChain> m_SwapChain;
		Ref<EventListener> m_EventListener;

		glm::ivec2 m_Position = glm::ivec2(0);
		bool m_IsMaximized = false;
		bool m_IsMinimized = false;

		std::function<void(int, int, bool&)> m_TitlebarHitTestCallback;

		uint16_t m_DownMouseButtons = 0;
		WINDOWPLACEMENT m_PreFullscreenWindowPlacement{};

		CursorMode m_CursorMode = CursorMode::Normal;
		glm::vec2 m_RestoreCursorPosition = glm::vec2(0.0f);

		glm::vec2 m_LastCursorPosition = glm::vec2(0.0f);
		glm::vec2 m_VirtualCursorPosition = glm::vec2(0.0f);
	};

}
