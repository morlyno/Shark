#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"

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
		void OnEvent(Event& e) override;

	};

}