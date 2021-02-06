#pragma once

#include "Shark/Event/Event.h"

namespace Shark {

	class ApplicationEvent : public Event
	{
	public:
		SK_GET_CATEGORY_FLAGS_FUNC(EventCategoryApplication);
	};

	class ApplicationCloseEvent : public ApplicationEvent
	{
	public:
		SK_EVENT_FUNCTIONS(ApplicationClosed)
	};

}