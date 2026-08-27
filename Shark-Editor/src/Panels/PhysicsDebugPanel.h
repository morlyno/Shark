#pragma once

#include "Panel.h"

namespace Shark {

	class PhysicsDebugPanel : public Panel
	{
	public:
		PhysicsDebugPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(const Ref<Scene>& context) override;

		static const char* GetStaticID() { return "PhysicsDebugPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		Ref<Scene> m_Scene;
	};

}
