#pragma once

#include "Shark/Editor/Panel.h"

namespace Shark {

	class ScriptEnginePanel : public Panel
	{
	public:
		ScriptEnginePanel(const char* panelName);

		virtual void OnImGuiRender(bool& shown) override;

	};

}
