#pragma once

#include "Shark/Scene/Scene.h"
#include "Shark/Event/Event.h"

#include "Panel.h"

namespace Shark {

	class PhysicsDebugPanel : public Panel
	{
	public:
		PhysicsDebugPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> scene) override { m_Scene = scene; }

		static const char* GetStaticID() { return "PhysicsDebugPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		Ref<Scene> m_Scene;
	};

}
