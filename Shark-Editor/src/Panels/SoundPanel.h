#pragma once

#include "Panel.h"

namespace Shark {

	class SoundPanel : public Panel
	{
	public:
		virtual void OnImGuiRender(bool& isOpen) override;

		static const char* GetStaticID() { return "SoundPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

	private:
		bool m_ShowActive = false;
		bool m_Simple = false;

	};

}
