#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class Panel : public RefCount
	{
	public:
		Panel(const char* panelName) : m_PanelName(panelName) {}
		virtual ~Panel() = default;

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnImGuiRender(bool& shown) {}
		virtual void OnEvent(Event& event) {}

		const char* GetName() const { return m_PanelName; }

	protected:
		const char* m_PanelName = nullptr;
	};

}
