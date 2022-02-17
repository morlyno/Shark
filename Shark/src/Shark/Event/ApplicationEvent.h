#pragma once

#include "Shark/Event/Event.h"

#include "Shark/File/FileWatcher.h"

namespace Shark {

	class ApplicationCloseEvent : public EventBase<EventTypes::ApplicationClosed, EventCategoryApplication>
	{
	public:
		ApplicationCloseEvent() = default;
	};

}