#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
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
	public:
		enum class SceneState
		{
			Edit = 0, Play = 1, Simulate = 2
		};
	public:
		EditorLayer(const std::filesystem::path& startupProject);
		~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnEvent(Event& event) override;

		virtual void OnImGuiRender() override;
	private:
		bool OnWindowResize(WindowResizeEvent& event);
		bool OnKeyPressed(KeyPressedEvent& event);

		void UI_MainMenuBar();
		void UI_Gizmo();
		void UI_Info();
		void UI_Shaders();
		void UI_EditorCamera();
		void UI_DragDrop();
		void UI_ToolBar();
		void UI_Settings();
		void UI_CameraPrevie();
		void UI_Stats();
		void UI_ProjectSettings();

		void SelectEntity(Entity entity);

		void NewScene();

		bool LoadScene(const std::filesystem::path& filePath);
		bool LoadScene();
		bool LoadScene(Ref<Scene> scene);
		bool SaveScene();
		bool SaveSceneAs();
		bool SerializeScene(Ref<Scene> scene, const std::filesystem::path& filePath);

		void OnScenePlay();
		void OnSceneStop();

		void OnSimulateStart();

		void SetActiveScene(const Ref<Scene>& scene);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filePath);
		void CloseProject();
		void SaveProject();
		void SaveProject(const std::filesystem::path& filePath);
		void CreateProject();

	private:
		std::filesystem::path m_StartupProject;

		EditorCamera m_EditorCamera;
		Ref<Image2D> m_MousePickingImage;

		Ref<SceneRenderer> m_SceneRenderer;
		Ref<SceneRenderer> m_CameraPreviewRenderer;
		Ref<Scene> m_ActiveScene = nullptr;
		Ref<Scene> m_WorkScene = nullptr;

		Ref<Scene> m_LoadedScene = nullptr;

		Scope<SceneHirachyPanel> m_SceneHirachyPanel;
		Scope<AssetsPanel> m_ContentBrowserPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;
		bool m_NeedsResize = true;

		TimeStep m_TimeStep;

		bool m_ShwoSceneHirachyPanel = true;
		bool m_ShowAssetsPanel = true;

		bool m_ShowInfo = true;
		bool m_ShowEditorCameraControlls = false;
		bool m_ShowSettings = true;
		bool m_ShowStats = true;
		bool m_ReadHoveredEntity = false;
		bool m_ShowShaders = false;
		bool m_ShowProjectSettings = false;

		int m_HoveredEntityID = -1;

		bool m_NegativeEffect = false;
		bool m_BlurEffect = false;

		float m_TranslationSnap = 0.5f;
		float m_RotationSnap = 45.0f;
		float m_ScaleSnap = 0.5f;
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

		std::string m_ProjectEditBuffer;
		bool m_ProjectEditActice = false;
		ImGuiID m_ProjectEditActiveID;
	};

}