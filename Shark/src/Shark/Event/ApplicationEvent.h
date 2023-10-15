#pragma once

#include "Shark/Event/Event.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class ApplicationClosedEvent : public EventBase<EventType::ApplicationClosed, EventCategory::Application>
	{
	};

}