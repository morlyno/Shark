#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Layer/Layer.h"

// Temporary
#ifndef SK_PLATFORM_WINDOWS
#error ImGui only supports windows at the moment
#endif

namespace Shark {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void Begin();
		void End();
		void OnEvent( Event& e ) override;

		void OnImGuiRender() override;

	};

}