#pragma once

#include "Panel.h"

namespace Shark {

	class ECSDebugPanel : public Panel
	{
	public:
		ECSDebugPanel(const std::string& name, Ref<Scene> context);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> context) override { m_Context = context; }

	private:
		Ref<Scene> m_Context;
	};

}
