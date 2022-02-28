#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Render/Texture.h>
#include <Shark/Render/SceneRenderer.h>
#include <Shark/Render/Renderer2D.h>

#include "SceneHirachyPanel.h"
#include "ContentBrowserPanel.h"

#include <ImGuizmo.h>

namespace Shark {

	class EditorLayer : public Layer
	{
	public:
		enum class SceneState
		{
			None = 0,
			Edit, Play, Simulate, Pause
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

		void OnFileChanged(const std::vector<FileChangedData>& fileEvents);

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
		void UI_Asset();

		void DebugRender();
		void RenderCameraPreview();

		void DeleteEntity(Entity entity);
		void SelectEntity(Entity entity);

		glm::mat4 GetActiveViewProjection() const;

		void NewScene();

		bool LoadScene(const std::filesystem::path& filePath);
		bool SaveScene();
		bool SaveSceneAs();
		bool SerializeScene(Ref<Scene> scene, const std::filesystem::path& filePath);

		void OnScenePlay();
		void OnSceneStop();

		void OnSimulateStart();

		void SetActiveScene(const Ref<Scene>& scene);
		void SetNextActiveScene(const Ref<Scene>& scene);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filePath);
		void CloseProject();
		void SaveProject();
		void SaveProject(const std::filesystem::path& filePath);
		void CreateProject();

		void ImportAsset();

		glm::mat4 GetViewProjFromCameraEntity(Entity cameraEntity);

	private:
		std::filesystem::path m_StartupProject;

		EditorCamera m_EditorCamera;
		Ref<Image2D> m_MousePickingImage;

		Ref<SceneRenderer> m_SceneRenderer;
		Ref<SceneRenderer> m_CameraPreviewRenderer;
		Ref<Renderer2D> m_DebugRenderer;

		Ref<Scene> m_ActiveScene = nullptr;
		Ref<Scene> m_WorkScene = nullptr;

		Ref<Scene> m_NextActiveScene = nullptr;

		Scope<SceneHirachyPanel> m_SceneHirachyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_ViewportSizeChanged = false;
		bool m_NeedsResize = true;

		TimeStep m_TimeStep;

		bool m_ShowSceneHirachyPanel = true;
		bool m_ShowAssetsPanel = true;

		bool m_ShowInfo = true;
		bool m_ShowEditorCameraControlls = false;
		bool m_ShowSettings = true;
		bool m_ShowStats = true;
		bool m_ReadHoveredEntity = false;
		bool m_ShowShaders = false;
		bool m_ShowProjectSettings = false;
		bool m_ShowAssetsRegistry = false;

		int m_HoveredEntityID = -1;

		bool m_NegativeEffect = false;
		bool m_BlurEffect = false;

		float m_TranslationSnap = 0.5f;
		float m_RotationSnap = 45.0f;
		float m_ScaleSnap = 0.5f;
		int m_CurrentOperation = 0;
		Entity m_SelectetEntity;

		SceneState m_SceneState = SceneState::Edit;
		SceneState m_InitialSceneState = SceneState::None;
		bool m_UpdateNextFrame = false;

		// ToolBar icons
		Ref<Texture2D> m_PlayIcon;
		Ref<Texture2D> m_StopIcon;
		Ref<Texture2D> m_SimulateIcon;
		Ref<Texture2D> m_PauseIcon;
		Ref<Texture2D> m_StepIcon;

		std::string m_ProjectEditBuffer;
		bool m_ProjectEditActice = false;
		ImGuiID m_ProjectEditActiveID;

		struct ImportAssetData
		{
			std::filesystem::path SourceFile;
			std::filesystem::path TargetDirectory;
			std::string FileName;
			std::string Extention;
			bool Active = false;
		};
		ImportAssetData m_ImportAssetData;

		bool m_ShowColliders = false;
		bool m_ShowCollidersOnTop = true;

		bool m_ShowCameraPreview = false;


		struct ProjectEditData
		{
			std::string Assets;
			std::string StartupScene;

			bool ValidAssetsPath = true;
			bool ValidStartupScene = true;

			ProjectEditData() = default;
			ProjectEditData(const ProjectConfig& config)
			{
				Assets = Project::RelativeCopy(config.AssetsDirectory).string();
				StartupScene = Project::RelativeCopy(config.StartupScenePath).string();
				ValidAssetsPath = true;
				ValidStartupScene = true;
			}
		};
		ProjectEditData m_ProjectEditData;

	};

}