#pragma once

#include <Shark/Render/EditorCamera.h>
#include <Shark/Render/FrameBuffer.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Render/Texture.h>
#include <Shark/Render/SceneRenderer.h>
#include <Shark/Render/Renderer2D.h>

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"
#include "Panels/EditorPanelManager.h"

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

		void OpenAssetCallback(AssetHandle assetHandle);

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
		void UI_ImportTexture();

		bool UI_MousePicking();

		void DebugRender();
		void RenderCameraPreview();

		Entity CreateEntity(const std::string& name = "Untitled");
		void DeleteEntity(Entity entity);
		void SelectEntity(Entity entity);

		glm::mat4 GetActiveViewProjection() const;

		void NewScene();

		bool LoadScene(const std::filesystem::path& filePath);
		bool SaveScene();
		bool SaveSceneAs();

		void OnScenePlay();
		void OnSceneStop();

		void OnSimulateStart();

		void SetActiveScene(Ref<Scene> scene);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filePath);
		void CloseProject();
		void SaveProject();
		void SaveProject(const std::filesystem::path& filePath);
		void CreateProject();

		glm::mat4 GetViewProjFromCameraEntity(Entity cameraEntity);

		void DistributeEvent(Event& event);

	private:
		std::filesystem::path m_StartupProject;

		EditorCamera m_EditorCamera;
		Ref<Image2D> m_MousePickingImage;

		Ref<SceneRenderer> m_SceneRenderer;
		Ref<SceneRenderer> m_CameraPreviewRenderer;
		Ref<Renderer2D> m_DebugRenderer;

		Ref<Scene> m_ActiveScene = nullptr;
		Ref<Scene> m_WorkScene = nullptr;

		bool m_ViewportHovered = false, m_ViewportFocused = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		ImRect m_ViewportBounds;
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

		struct TextureSourceImportData
		{
			std::string TextureSourcePath;
			std::string TextureFileName;

			bool OpenPopup = false;
			bool CreateEntityAfterCreation = false;

			void Clear()
			{
				TextureSourcePath.clear();
				TextureFileName.clear();
				OpenPopup = false;
				CreateEntityAfterCreation = false;
			}
		};
		TextureSourceImportData m_TextureAssetCreateData;

	};

}