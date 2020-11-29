#pragma once
#include "skpch.h"
#include "Core.h"
#include "Shark/Event/Event.h"

namespace Shark {

	struct SHARK_API WindowProps
	{
		int width;
		int height;
		std::wstring name;

		WindowProps( const std::wstring& name = L"Shark Engin",unsigned int width = 1280u,unsigned int height = 720u )
			: width( width ), height( height ), name( name )
		{}
	};

	class SHARK_API Window
	{
	public:
		using EventCallbackFunc = std::function<void( Event& e )>;

		virtual ~Window() = default;

		virtual void SetEventCallbackFunc( const EventCallbackFunc& callback ) = 0;

		virtual bool Process() const = 0;

		virtual inline int GetWidth() const = 0;
		virtual inline int GetHeight() const = 0;
		virtual inline SK_WINDOW_HANDLE GetWindowHandle() const = 0;

		virtual inline bool IsFocused() const = 0;

		static Window* Create( const WindowProps& properties = WindowProps() );
	};

}