#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Event/Event.h"
#include "Shark/Core/TimeStep.h"

namespace Shark {

	class Layer
	{
	public:
		Layer(const std::string& LayerName = "Layer")
			: LayerName(LayerName) {}
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnBeginFrame() {}
		virtual void OnEndFrame() {}
		virtual void OnUpdate(TimeStep t) {}

		virtual void OnBeginUI() {}
		virtual void OnEndUI() {}
		virtual void OnImGuiRender() {}

		virtual void OnEvent(Event& e) {}

		inline const std::string& GetName() const { return LayerName; }
	protected:
		std::string LayerName;
	};

}