#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Layer/Layer.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/EditorCamera.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/KeyEvent.h"

#include "Shark/File/FileWatcher.h"

#include "Panels/PanelManager.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <imgui_internal.h>

namespace Shark {

	class EditorLayer : public Layer
	{
	public:
		enum class SceneState
		{
			None = 0,
			Edit, Play, Simulate, Pause
		};

		struct GizmoOperaton
		{
			enum Type : std::underlying_type_t<ImGuizmo::OPERATION>
			{
				None = 0,
				Translate = ImGuizmo::TRANSLATE,
				Rotate = ImGuizmo::ROTATE,
				Scale = ImGuizmo::SCALE
			};
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
		bool OnKeyPressed(KeyPressedEvent& event);
		void OnFileEvents(const std::vector<FileChangedData>& fileEvents);
		void OnFileClickedCallback(const std::filesystem::path& filePath);

		void UI_MainMenuBar();
		void UI_Viewport();
		void UI_Gizmo();
		void UI_Info();
		void UI_Shaders();
		void UI_EditorCamera();
		void UI_DragDrop();
		void UI_ToolBar();
		void UI_CameraPrevie();
		void UI_ProfilerStats();
		void UI_ProjectSettings();
		void UI_ImportTexture();
		bool UI_MousePicking();
		void UI_DebugScripts();
		void UI_LogSettings();
		void UI_Statistics();

		void RegisterSettingNodes();

		void OpenAssetEditor(AssetHandle assetHandle);

		void DebugRender();
		void RenderCameraPreview();

		Entity CreateEntity(const std::string& name = "Untitled");
		void DeleteEntity(Entity entity);
		void SelectEntity(Entity entity);

		glm::mat4 GetActiveViewProjection() const;

		void NewScene(const std::string& name = "New Scene");

		bool LoadScene(AssetHandle handle);
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
		void SaveActiveProject();
		void SaveActiveProject(const std::filesystem::path& filePath);
		Ref<ProjectInstance> CreateProject(const std::filesystem::path& projectDirectory);
		void CreateProjectPremakeFile(Ref<ProjectInstance> project);

		void ImportAssetDialog();

		void RunScriptSetup();
		void OpenIDE();

		void UpdateWindowTitle();

	private:
		static constexpr std::string_view LogLevelStrings[] = { "Trace"sv, "Debug"sv, "Info"sv, "Warn"sv, "Error"sv, "Critical"sv/*, "Off"sv*/ };

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
		ImVec2 m_ViewportPos = ImVec2(0, 0);
		bool m_NeedsResize = true;

		TimeStep m_TimeStep;

		ImGuiID m_MainViewportID = 0;

		Scope<PanelManager> m_PanelManager;

		bool m_ShowInfo = false;
		bool m_ShowEditorCameraControlls = false;
		bool m_ShowStats = false;
		bool m_ReadHoveredEntity = false;
		bool m_ShowShaders = false;
		bool m_ShowProjectSettings = false;
		bool m_ShowThemeEditor = false;
		bool m_ShowLogSettings = false;
		bool m_ShowCreateProject = false;
		bool m_ShowDebugScripts = false;
		bool m_ShowStatistics = false;
		bool m_ReadPixel = false;
		glm::vec4 m_HoveredColor;

		int m_HoveredEntityID = -1;

		float m_TranslationSnap = 0.5f;
		float m_RotationSnap = 45.0f;
		float m_ScaleSnap = 0.5f;
		GizmoOperaton::Type m_CurrentOperation = GizmoOperaton::None;
		Entity m_SelectetEntity;

		SceneState m_SceneState = SceneState::Edit;
		SceneState m_InitialSceneState = SceneState::None;
		bool m_UpdateNextFrame = false;

		bool m_ShowColliders = false;
		bool m_ShowCameraPreview = false;

		struct ProjectEditData
		{
			std::string Assets;
			std::string StartupScene;

			bool ValidAssetsPath = true;
			bool ValidStartupScene = true;

			ProjectEditData() = default;
			ProjectEditData(Ref<ProjectInstance> project)
			{
				Assets = Project::RelativeCopy(project->AssetsDirectory).string();
				StartupScene = Project::RelativeCopy(project->StartupScenePath).string();
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

		bool m_ReloadEditorIcons = false;

		struct CreateProjectData
		{
			std::string Name = "Untitled";
			std::filesystem::path Directory;
		};
		CreateProjectData m_CreateProjectData;

		struct ProfilerEntry
		{
			std::string Descriptor;
			TimeStep Duration;
		};

		std::vector<ProfilerEntry> m_ProfilerStats;
		std::map<std::string, TimeStep> m_ProfilerStatsAccumulator;
		uint32_t m_ProfilerSamples = 10;
		uint32_t m_ProfilerSampleCount = 0;
	};

}