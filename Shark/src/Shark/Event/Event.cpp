#include "skpch.h"
#include "Event.h"

#include "Shark/Core/Application.h"

namespace Shark {

	void Event::Distribute(Event& event)
	{
		Application::Get().OnEvent(event);
	}

}
