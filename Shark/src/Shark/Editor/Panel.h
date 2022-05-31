#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Event/Event.h"

namespace Shark {

	class Panel : public RefCount
	{
	public:
		virtual ~Panel() = default;

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnImGuiRender(bool& shown) {}
		virtual void OnEvent(Event& event) {}
	};

}
