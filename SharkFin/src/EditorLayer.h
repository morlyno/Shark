#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
#include <Shark/Render/Rasterizer.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Render/Texture.h>
#include <Shark/Render/SceneRenderer.h>
#include <Shark/Render/Renderer2D.h>

#include "SceneHirachyPanel.h"
#include "AssetsPanel.h"

#include <ImGuizmo.h>

namespace Shark {

	class EditorLayer : public Layer
	{
		// Temp: Test Functions
		void EffectesTest();
	public:
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
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

		void UI_MainMenuBar();
		void UI_Gizmo();
		void UI_Info();
		void UI_Shaders();
		void UI_EditorCamera();
		void UI_Project();
		void UI_DragDrop();
		void UI_ToolBar();

		void NewScene();

		bool LoadScene();
		bool LoadScene(Ref<Scene> scene);
		bool SaveScene();
		bool SaveSceneAs();
		bool SerializeScene(Ref<Scene> scene, const std::filesystem::path& filePath);

		void OnScenePlay();
		void OnSceneStop();

		void OnSimulateStart();

		void SetActiveScene(const Ref<Scene>& scene);

	private:
		EditorCamera m_EditorCamera;
		//Ref<FrameBuffer> m_CompositFrameBuffer;
		Ref<Image2D> m_MousePickingImage;

		Ref<SceneRenderer> m_SceneRenderer;
		Ref<Scene> m_ActiveScene = nullptr;
		Ref<Scene> m_WorkScene = nullptr;
		
		SceneHirachyPanel m_SceneHirachyPanel;
		AssetsPanel m_AssetsPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;

		TimeStep m_TimeStep;

		bool m_ShowInfo = true;
		bool m_ShowEditorCameraControlls = false;
		bool m_ShowProject = false;

		int m_HoveredEntityID = -1;

		bool m_NegativeEffect = false;
		bool m_BlurEffect = false;

		int m_CurrentOperation = 0;
		Entity m_SelectetEntity;


		SceneState m_SceneState = SceneState::Edit;
		bool m_ScenePaused = false;

		// ToolBar icons
		Ref<Texture2D> m_PlayIcon;
		Ref<Texture2D> m_StopIcon;
		Ref<Texture2D> m_SimulateIcon;
		Ref<Texture2D> m_PauseIcon;
		Ref<Texture2D> m_StepIcon;

	};

}