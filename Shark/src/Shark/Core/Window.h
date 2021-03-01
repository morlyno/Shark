#pragma once
#include "Shark/Core/Base.h"
#include "Shark/Event/Event.h"

namespace Shark {

	struct WindowProps
	{
		std::wstring Name = L"Shark Game Engine\0";
		int Width = 1280;
		int Height = 720;
		bool VSync = true;

		WindowProps() = default;
		WindowProps(const std::wstring& name, int width, int height, bool VSync)
			: Name(name), Width(width), Height(height), VSync(VSync)
		{}
	};

	class Window
	{
	public:
		using EventCallbackFunc = std::function<void(Event& e)>;

		virtual ~Window() = default;

		virtual void SetEventCallbackFunc(const EventCallbackFunc& callback) = 0;

		virtual void Update() const = 0;

		virtual inline int GetWidth() const = 0;
		virtual inline int GetHeight() const = 0;
		virtual inline void* GetHandle() const = 0;

		virtual inline bool IsFocused() const = 0;

		virtual inline bool IsVSync() const = 0;
		virtual void SetVSync(bool VSync) = 0;

		virtual void Kill() = 0;

		static Scope<Window> Create(const WindowProps& properties = WindowProps());
	};

}