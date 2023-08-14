#pragma once

#include "Shark/Scene/Scene.h"
#include "Shark/Event/Event.h"

#include "Shark/Editor/Panel.h"

namespace Shark {

	class PhysicsDebugPanel : public Panel
	{
	public:
		PhysicsDebugPanel(const std::string& panelName);

		virtual void OnImGuiRender(bool& shown) override;

		virtual void SetContext(Ref<Scene> scene) override { m_Scene = scene; }

	private:
		Ref<Scene> m_Scene;
	};

}
