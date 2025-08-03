#pragma once

#include "Panel.h"

namespace Shark {

	class ShadersPanel : public Panel
	{
	public:
		ShadersPanel();
		~ShadersPanel();

		virtual void OnImGuiRender(bool& shown) override;

		static const char* GetStaticID() { return "ShadersPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		char m_SearchBuffer[260];
		bool m_DisableOptimization = false;
	};

}
