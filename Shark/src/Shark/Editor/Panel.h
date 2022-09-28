#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class Panel : public RefCount
	{
	public:
		Panel(const char* panelName) : PanelName(panelName) {}
		virtual ~Panel() = default;

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnImGuiRender(bool& shown) {}
		virtual void OnEvent(Event& event) {}

	public:
		const char* PanelName = nullptr;
	};

}
