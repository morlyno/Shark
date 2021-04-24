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
		SK_EVENT_FUNCTIONS(ApplicationClosed);
	};

	class Entity;
	class Scean;

	class SelectionChangedEvent : public ApplicationEvent
	{
	public:
		SelectionChangedEvent(Entity entity);

		SK_EVENT_FUNCTIONS(SelectionChanged);

		Entity GetSelectedEntity();
	private:
		uint32_t m_EntityID;
		Weak<Scean> m_Scean;
	};

}