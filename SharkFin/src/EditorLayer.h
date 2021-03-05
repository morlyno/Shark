#pragma once

#include <Shark.h>
#include <Shark/Render/EditorCamera.h>
#include "SceanHirachyPanel.h"

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

		void NewScean();
		void SaveScean();
		void LoadScean();
	private:
		EditorCamera m_EditorCamera;
		Ref<Texture2D> m_FrameBufferTexture;

		Ref<Scean> m_ActiveScean;
		SceanHirachyPanel m_SceanHirachyPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		DirectX::XMFLOAT2 m_ViewportSize = { 0.0f, 0.0f };
		bool m_ViewportSizeChanged = false;

		bool m_PlayScean = false;
	};

}