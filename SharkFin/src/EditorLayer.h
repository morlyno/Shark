#pragma once

#include <Shark.h>
#include <Shark/Render/EditorCamera.h>

#include "SceanHirachyPanel.h"
#include "AssetsPanel.h"

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

		void OnPlayScean();
		void OnStopScean();
	private:
		EditorCamera m_EditorCamera;
		Ref<SwapChain> m_SwapChain;
		Ref<FrameBuffer> m_SwapChainFrameBuffer;
		Ref<FrameBuffer> m_FrameBuffer;
		Ref<Topology> m_Topology;
		Ref<Rasterizer> m_Rasterizer;
		Ref<Rasterizer> m_HilightRasterizer;

		SceanController m_Scean;
		SceanHirachyPanel m_SceanHirachyPanel;
		AssetsPanel m_AssetsPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;

		bool m_PlayScean = false;
		bool m_ShowRendererStats = true;
		bool m_ShowSceanHirachyPanel = true;
		bool m_ShowEditorCameraControlls = true;

		int m_HoveredEntityID = -1;

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