#pragma once

#include <Shark.h>
#include "Shark/Render/EditorCamera.h"

namespace Shark {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnImGuiRender() override;
	private:
		bool OnWindowResize(WindowResizeEvent& event);
	private:
		EditorCamera m_EditorCamera;
		Ref<Texture2D> m_FrameBufferTexture;
	};

}