#pragma once

#include "Shark/Editor/Panel.h"

namespace Shark {

	class ScriptEnginePanel : public Panel
	{
	public:
		virtual void OnImGuiRender(bool& shown) override;

	private:
	};

}
