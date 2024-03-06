#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"

#include <imgui.h>

namespace Shark {

	class Image2D;

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

		virtual bool InFrame() const = 0;
		virtual void SetMainViewportID(ImGuiID mainViewportID) = 0;
		virtual ImGuiID GetMainViewportID() const = 0;

		virtual bool BlocksMouseEvents() const = 0;
		virtual bool BlocksKeyboardEvents() const = 0;
		virtual void BlockEvents(bool block) = 0;

		virtual void AddImage(Ref<Image2D> image) = 0;
		virtual void BindFontSampler() = 0;

		static ImGuiLayer* Create();
	};

}