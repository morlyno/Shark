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
		Panel() = default;
		virtual ~Panel() = default;

		// By returning false Showing/Hiding the panel is canceled
		virtual bool OnShowPanel() { return true; }
		virtual bool OnHidePanel() { return true; }

		virtual void OnUpdate(TimeStep ts) {}
		virtual void OnImGuiRender(bool& isOpen) {}
		virtual void OnEvent(Event& event) {}

		virtual void SetContext(Ref<Scene> context) {}
		virtual void OnScenePlay() {};
		virtual void OnSceneStop() {};
		virtual void OnProjectChanged(Ref<ProjectConfig> projectConfig) {}

		virtual const char* GetPanelID() const = 0;

		const char* GetName() const { return m_PanelName; }
		void SetName(const char* name) { m_PanelName = name; }

	protected:
		const char* m_PanelName;
	};

}
