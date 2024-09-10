#pragma once

#include "Panel.h"

namespace Shark {

	class IconSelector : public Panel
	{
	public:
		IconSelector(const std::string& name);
		~IconSelector();

		virtual void OnImGuiRender(bool& shown) override;
	};

}
