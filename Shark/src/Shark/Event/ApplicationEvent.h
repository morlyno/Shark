#pragma once

#include "Shark/Event/Event.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class ApplicationCloseEvent : public EventBase<EventTypes::ApplicationClosed, EventCategoryApplication>
	{
	public:
		ApplicationCloseEvent() = default;
	};

	class SceneChangedEvent : public EventBase<EventTypes::SceneChanged, EventCategoryApplication>
	{
	public:
		SceneChangedEvent(Ref<Scene> scene)
			: m_Scene(scene)
		{}

		Ref<Scene> GetScene() const { return m_Scene; }

	private:
		Ref<Scene> m_Scene;
	};

	class ProjectChangedEvnet : public EventBase<EventTypes::ProjectChanged, EventCategoryApplication>
	{
	public:
		ProjectChangedEvnet(Ref<Project> project)
			: m_Project(project)
		{}

		Ref<Project> GetProject() const { return m_Project; }

	private:
		Ref<Project> m_Project;
	};

}