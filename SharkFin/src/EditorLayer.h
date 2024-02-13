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

#include "Shark/File/FileSystem.h"

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
			Edit, Play, Simulate
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
		bool OnWindowDropEvent(WindowDropEvent& event);

		void UI_MainMenuBar();
		void UI_Viewport();
		void UI_Gizmo();
		void UI_Info();
		void UI_EditorCamera();
		void UI_DragDrop();
		void UI_ToolBar();
		void UI_CameraPrevie();
		void UI_ProfilerStats();
		void UI_ImportTexture();
		bool UI_MousePicking();
		void UI_LogSettings();
		void UI_Statistics();
		void UI_OpenProjectModal();
		void UI_ImportAsset();
		void UI_CreateMeshAsset();

		void RegisterSettingNodes();

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

		void OnSimulationPlay();
		void OnSimulationStop();

		void SubmitOnScenePlay();
		void SubmitOnSceneStop();
		void SubmitOnSimulationPlay();
		void SubmitOnSimulationStop();
		void SubmitSetScenePaused(bool paused);
		void SubmitStepScene(uint32_t frames);

		void SetActiveScene(Ref<Scene> scene);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filePath);
		void CloseProject();
		Ref<Project> CreateProject(const std::filesystem::path& projectDirectory);
		void CreateProjectPremakeFile(Ref<Project> project);

		void RunScriptSetup();
		void OpenIDE();
		void AssembliesReloadedHook();

		void UpdateWindowTitle();

		void InstantiateMesh(Ref<Mesh> mesh);
		void InstantiateMeshNode(Ref<Mesh> mesh, const MeshNode& node, Entity parent);

		void VerifyEditorTexture(const std::filesystem::path& filepath);
		void VerifyEditorTexture(const std::filesystem::path& filepath, const std::filesystem::path& sourcePath);
		void VerifyAllEditorAssets();

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
		bool m_ReadHoveredEntity = false;
		bool m_ShowThemeEditor = false;
		bool m_ShowLogSettings = false;
		bool m_ShowCreateProject = false;
		bool m_ShowStatistics = false;
		bool m_ReadPixel = false;
		glm::vec4 m_HoveredColor;

		int m_HoveredEntityID = -1;

		float m_TranslationSnap = 0.5f;
		float m_RotationSnap = 45.0f;
		float m_ScaleSnap = 0.5f;
		GizmoOperaton::Type m_CurrentOperation = GizmoOperaton::None;
		bool m_RenderGizmo = true;

		Entity m_SelectetEntity;

		SceneState m_SceneState = SceneState::Edit;

		bool m_ShowColliders = false;
		bool m_ShowCameraPreview = false;

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

		struct OpenProjectModal
		{
			bool Show = false;
			std::filesystem::path ProjectFile;
			bool OpenPopup = false;

			void Open(const std::filesystem::path& projectFile)
			{
				Show = true;
				ProjectFile = projectFile;
				OpenPopup = true;
			}

			void Reset()
			{
				Show = false;
				ProjectFile = "";
				OpenPopup = false;
			}
		};
		OpenProjectModal m_OpenProjectModal;

		struct ImportAssetData
		{
			bool Show = false;
			AssetType Type = AssetType::None;
			std::string DestinationPath;
			std::string SourcePath;

			std::string Error;
			bool ShowError = false;
			float ShowErrorTimer = 0.0f;
			static constexpr float ShowErrorDuration = 4.0f;
		};
		ImportAssetData m_ImportAssetData;

		std::unordered_map<AssetType, std::string> m_DefaultAssetDirectories = {
			{ AssetType::Scene, "Scenes" },
			{ AssetType::Texture, "Textures" },
			{ AssetType::TextureSource, "TextureSources" },
			{ AssetType::ScriptFile, "Scripts/Source" },
			{ AssetType::Font, "Fonts" },
			{ AssetType::MeshSource, "MeshSources" },
			{ AssetType::Mesh, "Meshes" },
			{ AssetType::Material, "Material" }
		};

		bool m_ShaderCompilerDisableOptimization = false;

		struct CreateMeshAssetData
		{
			bool Show = false;
			AssetHandle MeshSource = AssetHandle::Invalid;
			std::string DestinationPath;
			std::string MeshDirectory;
		};
		CreateMeshAssetData m_CreateMeshAssetData;

	};

}