#pragma once

#include "Panel.h"

namespace Shark {

	class IconSelector : public Panel
	{
	public:
		IconSelector();
		~IconSelector();

		virtual void OnImGuiRender(bool& shown) override;

		static const char* GetStaticID() { return "IconSelector"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	};

}
