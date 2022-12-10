#pragma once

#include "Shark/Event/Event.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class ApplicationClosedEvent : public EventBase<Event, EventType::ApplicationClosed, EventCategory::Application>
	{
	public:
		ApplicationClosedEvent() = default;
	};

#if 0
	class SceneChangedEvent : public EventBase<Event, EventType::SceneChanged, EventCategory::Application>
	{
	public:
		SceneChangedEvent(Ref<Scene> scene)
			: m_Scene(scene)
		{}

		Ref<Scene> GetScene() const { return m_Scene; }

	private:
		Ref<Scene> m_Scene;
	};

	class ScenePlayEvent : public EventBase<Event, EventType::ScenePlay, EventCategory::Application>
	{
	public:
		ScenePlayEvent(Ref<Scene> scene)
			: m_Scene(scene)
		{}

		Ref<Scene> GetScene() const { return m_Scene; }

	private:
		Ref<Scene> m_Scene;
	};

	class ProjectChangedEvent : public EventBase<Event, EventType::ProjectChanged, EventCategory::Application>
	{
	public:
		ProjectChangedEvent(Ref<ProjectInstance> project)
			: m_Project(project)
		{}

		Ref<ProjectInstance> GetProject() const { return m_Project; }

	private:
		Ref<ProjectInstance> m_Project;
	};
#endif

}