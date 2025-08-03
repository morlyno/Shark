#pragma once

#include "Shark/Render/SceneRenderer.h"

#include "Panel.h"

namespace Shark {

	class SceneRendererPanel : public Panel
	{
	public:
		SceneRendererPanel();
		virtual void OnImGuiRender(bool& shown) override;

		void SetRenderer(Ref<SceneRenderer> renderer);

		static const char* GetStaticID() { return "SceneRendererPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		Ref<SceneRenderer> m_Renderer;

		glm::vec4 m_ClearColor;
	};

}
