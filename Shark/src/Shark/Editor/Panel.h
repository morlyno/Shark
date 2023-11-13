#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Event/Event.h"
#include "Shark/Scene/Scene.h"

namespace Shark {

	class Panel : public RefCount
	{
	public:
		Panel(const std::string& panelName) : m_PanelName(panelName) {}
		virtual ~Panel() = default;

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnImGuiRender(bool& shown) {}
		virtual void OnEvent(Event& event) {}

		virtual void SetContext(Ref<Scene> context) {}
		virtual void OnScenePlay() {};
		virtual void OnSceneStop() {};
		virtual void OnProjectChanged(Ref<Project> project) {}

		const std::string& GetName() const { return m_PanelName; }

	protected:
		std::string m_PanelName;
	};

}
