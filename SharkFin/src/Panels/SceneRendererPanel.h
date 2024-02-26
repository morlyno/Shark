#pragma once

#include "Shark/Render/SceneRenderer.h"

#include "Panel.h"

namespace Shark {

	class SceneRendererPanel : public Panel
	{
	public:
		SceneRendererPanel(const std::string& panelName);
		virtual void OnImGuiRender(bool& shown) override;

		void SetRenderer(Ref<SceneRenderer> renderer) { m_Renderer = renderer; }
	private:
		Ref<SceneRenderer> m_Renderer;
		bool m_VSync = true;
	};

}
