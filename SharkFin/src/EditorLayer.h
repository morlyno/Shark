#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
#include <Shark/Render/Rasterizer.h>
#include <Shark/Scean/Scean.h>
#include <Shark/Scean/Entity.h>
#include <Shark/Scean/SceanController.h>
#include <Shark/Render/Texture.h>

#include "SceanHirachyPanel.h"
#include "AssetsPanel.h"

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
		Ref<FrameBuffer> m_FrameBuffer;
		Ref<Rasterizer> m_Rasterizer;
		Ref<Rasterizer> m_HilightRasterizer;

		SceanController m_Scean;
		SceanHirachyPanel m_SceanHirachyPanel;
		AssetsPanel m_AssetsPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;

		bool m_PlayScean = false;

		bool m_ShowInfo = false;
		bool m_ShowEditorCameraControlls = false;

		int m_HoveredEntityID = -1;

		TimeStep m_TimeStep;
	};

}