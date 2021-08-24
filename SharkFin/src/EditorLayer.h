#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
#include <Shark/Render/Rasterizer.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Scene/SceneController.h>
#include <Shark/Render/Texture.h>

#include "SceneHirachyPanel.h"
#include "AssetsPanel.h"

#include <ImGuizmo.h>

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
		bool OnSelectionChanged(SelectionChangedEvent& event);

		void OnImGuiRender_Project();

		void NewScene();
		void SaveScene();
		void OpenScene();

		void SetActiveScene(Ref<Scene> scene);
		void LoadScene();
		void LoadNewScene(const std::filesystem::path& filepath);

		void OnPlayScene();
		void OnStopScene();
	private:
		EditorCamera m_EditorCamera;
		Ref<FrameBuffer> m_GemometryFrameBuffer;
		Ref<FrameBuffer> m_NegativeFrameBuffer;
		Ref<FrameBuffer> m_BlurFrameBuffer;
		Ref<FrameBuffer> m_CompositFrameBuffer;
		Ref<Rasterizer> m_Rasterizer;
		Ref<Rasterizer> m_HilightRasterizer;

		SceneController m_ActiveScene;
		SceneHirachyPanel m_SceneHirachyPanel;
		AssetsPanel m_AssetsPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;

		bool m_PlayScene = false;

		bool m_ShowInfo = true;
		bool m_ShowEditorCameraControlls = false;
		bool m_ShowProject = true;

		int m_HoveredEntityID = -1;

		TimeStep m_TimeStep;

		bool m_NegativeEffect = false;
		bool m_BlurEffect = false;

		int m_CurrentOperation = 0;
		Entity m_SelectetEntity;
	};

}