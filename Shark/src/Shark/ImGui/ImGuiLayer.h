#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"

namespace Shark {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();

		virtual ~ImGuiLayer() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;

		virtual void OnEvent(Event& event) = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void BlockEvents(bool block) = 0;
		virtual void SubmitBlendCallback(bool blend) = 0;

		virtual void SetDarkStyle() = 0;

	};

	ImGuiLayer* CreateImGuiLayer();

}