#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		
		virtual void OnEvent(Event& event) override;
		
		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetDarkStyle();
	private:
		bool m_BlockEvents = false;
	};

}