#pragma once

#include "Panel.h"

namespace Shark {

	class ShadersPanel : public Panel
	{
	public:
		ShadersPanel(const std::string& panelName);
		~ShadersPanel();

		virtual void OnImGuiRender(bool& shown) override;

	private:
		char m_SearchBuffer[260];
		bool m_DisableOptimization = false;
	};

}
