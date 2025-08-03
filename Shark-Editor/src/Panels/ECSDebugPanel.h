#pragma once

#include "Panel.h"

namespace Shark {

	class ECSDebugPanel : public Panel
	{
	public:
		ECSDebugPanel(Ref<Scene> context);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> context) override { m_Context = context; }

		static const char* GetStaticID() { return "ECSDebugPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		Ref<Scene> m_Context;
	};

}
