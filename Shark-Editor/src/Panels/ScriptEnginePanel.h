#pragma once

#include "Panel.h"

namespace Shark {

	class ScriptEnginePanel : public Panel
	{
	public:
		ScriptEnginePanel(const std::string& panelName);

		virtual void OnImGuiRender(bool& shown) override;

	};

}
