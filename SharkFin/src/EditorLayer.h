#pragma once

#include <Shark.h>
#include <Shark/Render/EditorCamera.h>

#include "SceanHirachyPanel.h"

#include <box2d/box2d.h>

//#define SHARK_BOX2D_TEST

namespace Shark {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnImGuiRender() override;
	private:
		bool OnWindowResize(WindowResizeEvent& event);
		bool OnKeyPressed(KeyPressedEvent& event);

		void NewScean();
		void SaveScean();
		void OpenScean();
	private:
		EditorCamera m_EditorCamera;
		Ref<FrameBuffer> m_FrameBuffer;
		Ref<Viewport> m_Viewport;

		Ref<Scean> m_ActiveScean;
		SceanHirachyPanel m_SceanHirachyPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;

		bool m_PlayScean = false;
		bool m_ShowRendererStats = true;
		bool m_ShowSceanHirachyPanel = true;
		bool m_ShowEditorCameraControlls = true;

		Ref<Texture2D> m_FrameBufferTexture;

		// Box2D Test
#ifdef SHARK_BOX2D_TEST
		b2World* m_World;
		b2Body* m_Groundbody;
		b2Body* m_DynamicBody;
		b2Body* m_DynamicBody1;
#endif
	};

}