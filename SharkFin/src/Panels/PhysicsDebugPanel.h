#pragma once

#include "Shark/Scene/Scene.h"
#include "Shark/Event/Event.h"

#include "Shark/Editor/Panel.h"

namespace Shark {

	class PhysicsDebugPanel : public Panel
	{
	public:
		PhysicsDebugPanel(const char* panelName);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		void SetContext(const Ref<Scene>& scene) { m_Scene = scene; }

	private:
		Ref<Scene> m_Scene;
	};

}
