#include "skfpch.h"
#include "EditorLayer.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/Memory.h"

#include "Shark/Scene/Components.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/Serialization/ProjectSerializer.h"
#include "Shark/Serialization/Import/AssimpMeshImporter.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/UI/UI.h"

#include "Icons.h"
#include "EditorSettings.h"

#include "Panel.h"
#include "Panels/EditorConsolePanel.h"
#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Panels/AssetEditorPanel.h"
#include "Panels/Editors/TextureEditorPanel.h"
#include "Panels/Editors/MaterialEditorPanel.h"
#include "Panels/PhysicsDebugPanel.h"
#include "Panels/ScriptEnginePanel.h"
#include "Panels/AssetsPanel.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/ShadersPanel.h"
#include "Panels/SceneRendererPanel.h"
#include "Panels/StatisticsPanel.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Debug/enttDebug.h"

#include <fmt/printf.h>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#define SCENE_HIRACHY_PANEL_ID "SceneHirachyPanel"
#define CONTENT_BROWSER_PANEL_ID "ContentBrowserPanel"
#define PHYSICS_DEBUG_PANEL_ID "PhysicsDebugPanel"
#define ASSET_EDITOR_MANAGER_ID "AssetsEditorManagerPanel"
#define EDITOR_CONSOLE_PANEL_ID "EditorConsolePanel"
#define SCRIPT_ENGINE_PANEL_ID "ScriptEnginePanel"
#define ASSETS_PANEL_ID "AssetsPanel"
#define SETTINGS_PANEL_ID "SettingsPanel"
#define PROJECT_SETTINGS_PANEL_ID "ProjectSettingsPanel"
#define SHADERS_PANEL_ID "ShadersPanel"
#define SCENE_RENDERER_PANEL_ID "SceneRendererPanel"
#define STATISTICS_PANEL_ID "StatisticsPanel"

namespace Shark {

	static bool s_ShowDemoWindow = false;

	EditorLayer::EditorLayer(const std::filesystem::path& startupProject)
		: Layer("EditorLayer"), m_StartupProject(startupProject)
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();

		Icons::Init();
		EditorSettings::Init();

		m_PanelManager = Scope<PanelManager>::Create();

		Ref<SceneHirachyPanel> sceneHirachy = m_PanelManager->AddPanel<SceneHirachyPanel>(PanelCategory::View, SCENE_HIRACHY_PANEL_ID, "Scene Hirachy", true);
		sceneHirachy->SetSelectionChangedCallback([this](Entity entity) { m_SelectetEntity = entity; });

		Ref<ContentBrowserPanel> contentBrowser = m_PanelManager->AddPanel<ContentBrowserPanel>(PanelCategory::View, CONTENT_BROWSER_PANEL_ID, "Content Browser", true);
		contentBrowser->RegisterOpenAssetCallback(AssetType::Material, [this](const AssetMetaData& metadata)
		{
			m_PanelManager->ShowPanel(ASSET_EDITOR_MANAGER_ID, true);
			auto assetEditorManager = m_PanelManager->Get<AssetEditorManagerPanel>(ASSET_EDITOR_MANAGER_ID);
			assetEditorManager->AddEditor<MaterialEditorPanel>(metadata);
		});

		contentBrowser->RegisterOpenAssetCallback(AssetType::Texture, [this](const AssetMetaData& metadata)
		{
			m_PanelManager->ShowPanel(ASSET_EDITOR_MANAGER_ID, true);
			auto assetEditorManager = m_PanelManager->Get<AssetEditorManagerPanel>(ASSET_EDITOR_MANAGER_ID);
			assetEditorManager->AddEditor<TextureEditorPanel>(metadata);
		});

		m_PanelManager->AddPanel<AssetEditorManagerPanel>(PanelCategory::Edit, ASSET_EDITOR_MANAGER_ID, "Assets Editor Manager", true);
		m_PanelManager->AddPanel<ProjectSettingsPanel>(PanelCategory::Edit, PROJECT_SETTINGS_PANEL_ID, "Project Settings", false, Project::GetActive());
		m_PanelManager->AddPanel<AssetsPanel>(PanelCategory::View, ASSETS_PANEL_ID, "Assets", false);
		m_PanelManager->AddPanel<EditorConsolePanel>(PanelCategory::View, EDITOR_CONSOLE_PANEL_ID, "Console", true);
		m_PanelManager->AddPanel<ShadersPanel>(PanelCategory::View, SHADERS_PANEL_ID, "Shaders", false);
		m_PanelManager->AddPanel<PhysicsDebugPanel>(PanelCategory::View, PHYSICS_DEBUG_PANEL_ID, "Pyhsics Debug", false);
		m_PanelManager->AddPanel<ScriptEnginePanel>(PanelCategory::View, SCRIPT_ENGINE_PANEL_ID, "Script Engine", false);
		m_PanelManager->AddPanel<SceneRendererPanel>(PanelCategory::View, SCENE_RENDERER_PANEL_ID, "Scene Renderer", true);
		m_PanelManager->AddPanel<StatisticsPanel>(PanelCategory::View, STATISTICS_PANEL_ID, "Statistics", false);

		const auto& window = Application::Get().GetWindow();
		m_SceneRenderer = Ref<SceneRenderer>::Create(window.GetWidth(), window.GetHeight(), "Viewport Renderer");
		m_PanelManager->Get<SceneRendererPanel>(SCENE_RENDERER_PANEL_ID)->SetRenderer(m_SceneRenderer);

		Renderer2DSpecifications debugRendererSpec;
		debugRendererSpec.UseDepthTesting = true;
		m_DebugRenderer = Ref<Renderer2D>::Create(m_SceneRenderer->GetExternalCompositFrameBuffer(), debugRendererSpec);

		// Readable image for Mouse Picking
		ImageSpecification imageSpecs = m_SceneRenderer->GetIDImage()->GetSpecification();
		imageSpecs.Type = ImageType::Storage;
		m_MousePickingImage = Image2D::Create(imageSpecs);

		// Load Project
		if (!m_StartupProject.empty())
			OpenProject(m_StartupProject);
		else
			SK_CORE_VERIFY(false, "No Startup Project!");

		auto& app = Application::Get();
		if (app.GetSpecification().EnableImGui)
		{
			m_MainViewportID = ImHashStr("MainViewport");
			app.GetImGuiLayer().SetMainViewportID(m_MainViewportID);
		}

		Renderer::WaitAndRender();
		ScriptEngine::RegisterAssembliesReloadedHook(std::bind(&EditorLayer::AssembliesReloadedHook, this));
		VerifyAllEditorAssets();
	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		m_PanelManager->Clear();

		CloseProject();

		Icons::Shutdown();
		EditorSettings::Shutdown();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		if (m_ActiveScene->IsFlagSet(AssetFlag::Invalid))
		{
			Ref<Scene> scene = AssetManager::GetAsset<Scene>(m_ActiveScene->Handle);
			if (scene)
			{
				SK_CORE_INFO("Scene Reload Detected");
				m_WorkScene = scene;
				SetActiveScene(scene);
			}
		}

		if (m_ActiveScene)
		{
			if (m_NeedsResize && m_ViewportWidth != 0 && m_ViewportHeight != 0)
			{
				SK_PROFILE_SCOPED("EditorLayer::OnUpdate Resize");

				m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);

				m_SceneRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
				m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);

				m_MousePickingImage->GetSpecification().Width = m_ViewportWidth;
				m_MousePickingImage->GetSpecification().Height = m_ViewportHeight;
				m_MousePickingImage->Invalidate();

				m_NeedsResize = false;
			}

			ImGui::GetIO().SetAppAcceptingEvents(Input::GetCursorMode() != CursorMode::Locked);
			m_EditorCamera.OnUpdate(ts, m_ViewportHovered && m_SceneState != SceneState::Play || Input::GetCursorMode() == CursorMode::Hidden);

			switch (m_SceneState)
			{
				case SceneState::Edit:
				{
					m_ActiveScene->OnUpdateEditor(ts);
					m_ActiveScene->OnRenderEditor(m_SceneRenderer, m_EditorCamera);
					break;
				}
				case SceneState::Play:
				{
					m_ActiveScene->OnUpdateRuntime(ts);
					m_ActiveScene->OnRenderRuntime(m_SceneRenderer);
					break;
				}
				case SceneState::Simulate:
				{
					m_ActiveScene->OnUpdateSimulate(ts);
					m_ActiveScene->OnRenderSimulate(m_SceneRenderer, m_EditorCamera);
					break;
				}
			}

			DebugRender();
		}

		m_PanelManager->OnUpdate(ts);
	}

	void EditorLayer::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispacher.DispachEvent<WindowDropEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowDropEvent));

		if (event.Handled)
			return;

		if (m_ViewportHovered && m_SceneState != SceneState::Play)
			m_EditorCamera.OnEvent(event);

		m_PanelManager->OnEvent(event);
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		SK_PROFILE_FUNCTION();

		if (event.GetKeyCode() == KeyCode::F4 && event.GetModifierKeys().Alt)
		{
			auto& app = Application::Get();
			app.CloseApplication();
			return true;
		}

		// Disable hot keys when the scene state is not Edit
		if (m_SceneState != SceneState::Edit)
		{
			if (event.GetKeyCode() == KeyCode::F && m_SelectetEntity)
			{
				glm::vec3 translation;
				Math::DecomposeTranslation(m_ActiveScene->GetWorldSpaceTransformMatrix(m_SelectetEntity), translation);

				m_EditorCamera.SetFocusPoint(translation);
				m_EditorCamera.SetDistance(7.5f);
				return true;
			}

			return false;
		}

		if (event.IsRepeat())
			return false;

		const bool control = event.GetModifierKeys().Control;
		const bool shift = event.GetModifierKeys().Shift;
		const bool alt = event.GetModifierKeys().Alt;

		switch (event.GetKeyCode())
		{

			// New Scene
			case KeyCode::N:
			{
				if (control)
				{
					NewScene();
					return true;
				}
				break;
			}

			// Save Scene
			case KeyCode::S:
			{
				if (control)
				{
					if (shift)
					{
						SaveSceneAs();
						return true;
					}
					SaveScene();
					return true;
				}
				break;
			}

			// Copy Entity
			case KeyCode::D:
			{
				if (control && m_SelectetEntity && m_SceneState == SceneState::Edit)
				{
					Entity e = m_WorkScene->CloneEntity(m_SelectetEntity);
					SelectEntity(e);
					return true;
				}
				break;
			}

			// Focus Selected Entity
			case KeyCode::F:
			{
				if (m_SelectetEntity)
				{
					glm::vec3 translation;
					Math::DecomposeTranslation(m_ActiveScene->GetWorldSpaceTransformMatrix(m_SelectetEntity), translation);

					m_EditorCamera.SetFocusPoint(translation);
					m_EditorCamera.SetDistance(7.5f);
					return true;
				}
				break;
			}

			case KeyCode::Delete:
			{
				if (m_SelectetEntity && m_ViewportFocused)
				{
					DeleteEntity(m_SelectetEntity);
					return true;
				}
				break;
			}

			// Toggle VSync
			case KeyCode::V:
			{
				auto& window = Application::Get().GetWindow();
				window.EnableVSync(!window.VSyncEnabled());
				return true;
			}

			// ImGuizmo
			case KeyCode::Q: { if (!Input::IsMouseDown(MouseButton::Right)) m_CurrentOperation = GizmoOperaton::None; return true; }
			case KeyCode::W: { if (!Input::IsMouseDown(MouseButton::Right)) m_CurrentOperation = GizmoOperaton::Translate; return true; }
			case KeyCode::E: { if (!Input::IsMouseDown(MouseButton::Right)) m_CurrentOperation = GizmoOperaton::Rotate; return true; }
			case KeyCode::R: { if (!Input::IsMouseDown(MouseButton::Right)) m_CurrentOperation = GizmoOperaton::Scale; return true; }
		}

		return false;
	}

	bool EditorLayer::OnWindowDropEvent(WindowDropEvent& event)
	{
		SK_PROFILE_FUNCTION();

		for (const auto& path : event.GetPaths())
		{
			if (!std::filesystem::is_regular_file(path))
				break;

			if (path.extension() == ".skproj")
			{
				m_OpenProjectModal.Open(path);
				break;
			}

			AssetType assetType = AssetUtils::GetAssetTypeFromPath(path);
			if (assetType != AssetType::None)
			{
				m_ImportAssetData.Show = true;
				m_ImportAssetData.Type = assetType;
				m_ImportAssetData.SourcePath = path.generic_string();
				m_ImportAssetData.DestinationPath = fmt::format("{}/{}", m_DefaultAssetDirectories.at(assetType), path.stem().generic_string());
				break;
			}

		}

		return false;
	}

	void EditorLayer::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		if (m_ReloadEditorIcons)
		{
			Icons::Reload();
			m_ReloadEditorIcons = false;
		}

		const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Shark Editor DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		ImGui::End();

		UI_MainMenuBar();
		UI_Viewport();
		UI_MousePicking();

		UI_ToolBar();

		UI_EditorCamera();
		UI_ImportTexture();
		UI_LogSettings();
		UI_OpenProjectModal();
		UI_ImportAsset();
		UI_CreateMeshAsset();

		m_PanelManager->OnImGuiRender();
		
		Theme::DrawThemeEditor(m_ShowThemeEditor);

		if (s_ShowDemoWindow)
			ImGui::ShowDemoWindow(&s_ShowDemoWindow);

	}

	void EditorLayer::UI_MainMenuBar()
	{
		SK_PROFILE_FUNCTION();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				switch (m_SceneState)
				{
					case SceneState::Edit:
					{
						if (ImGui::MenuItem("Play Scene"))
							OnScenePlay();
						break;
					}
					case SceneState::Play:
					{
						if (ImGui::MenuItem("Stop Scene"))
							OnSceneStop();
						break;
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New Scene", "ctrl+N"))
					NewScene();
				if (ImGui::MenuItem("Save Scene", "ctrl+S"))
					SaveScene();
				if (ImGui::MenuItem("Save Scene As", "ctrl+sift+S"))
					SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Create Project"))
				{
					auto projectDirectory = Platform::SaveDirectoryDialog();
					if (!projectDirectory.empty())
					{
						auto project = CreateProject(projectDirectory);
						OpenProject(project->GetProjectFilePath());
						//SetProject(project);
					}
				}

				if (ImGui::MenuItem("Open Project"))
					OpenProject();

				if (ImGui::BeginMenu("Recent Projects"))
				{
					// TODO(moro): Add Recent Projects
					ImGui::MenuItem("Sandbox");
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Save Project"))
					Project::SaveActive();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			m_PanelManager->DrawPanelsMenu();

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Theme Editor", nullptr, &m_ShowThemeEditor);

				if (ImGui::MenuItem("Reload Icons"))
					m_ReloadEditorIcons = true;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::Separator();
				ImGui::MenuItem("Log Settings", nullptr, &m_ShowLogSettings);
				ImGui::Separator();

				auto& app = Application::Get();
				if (ImGui::MenuItem("Fullscreen", nullptr, app.GetWindow().IsFullscreen()))
				{
					Application::Get().SubmitToMainThread([]()
					{
						auto& window = Application::Get().GetWindow();
						const bool nextMode = !window.IsFullscreen();
						window.SetFullscreen(nextMode);
						//window.GetSwapChain()->Resize(window.GetWidth(), window.GetHeight());
					});
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload"))
					ScriptEngine::ScheduleReload();

				ImGui::Separator();

				if (ImGui::MenuItem("Run Setup"))
					RunScriptSetup();

				if (ImGui::MenuItem("Open IDE"))
					OpenIDE();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("ImGui Demo Window", nullptr, &s_ShowDemoWindow);
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

	}

	void EditorLayer::UI_Viewport()
	{
		SK_PROFILE_FUNCTION();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginEx("MainViewport", m_MainViewportID, nullptr, ImGuiWindowFlags_None);
		ImGui::PopStyleVar(4);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		const bool anyItemFocused = ImGui::IsAnyItemActive() && GImGui->ActiveId != ImGui::GetCurrentWindow()->MoveId;

		if (Input::GetCursorMode() == CursorMode::Normal)
			Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered || anyItemFocused);
		else
			Application::Get().GetImGuiLayer().BlockEvents(false);


		const ImVec2 size = ImGui::GetContentRegionAvail();
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportPos = ImGui::GetWindowPos();
			m_NeedsResize = true;
		}

		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalImage();
		ImGui::Image(fbImage->GetViewID(), size);

		UI_Gizmo();
		UI_DragDrop();

		ImGui::End();
	}

	void EditorLayer::UI_Gizmo()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_RenderGizmo)
			return;

		if (m_CurrentOperation != GizmoOperaton::None && m_SelectetEntity)
		{
			SK_CORE_ASSERT(m_SelectetEntity.AllOf<TransformComponent>(), "Every entity is requiert to have a Transform Component");

			ImGuiWindow* window = ImGui::GetCurrentWindow();

			ImVec2 windowPos = window->WorkRect.Min;
			ImVec2 windowSize = window->WorkRect.GetSize();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

			glm::mat4 view;
			glm::mat4 projection;
			if (m_SceneState == SceneState::Play)
			{
				Entity cameraEntity = m_ActiveScene->GetActiveCameraEntity();
				SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
				glm::mat4 transform = cameraEntity.Transform().CalcTransform();

				ImGuizmo::SetOrthographic(camera.GetProjectionType() == SceneCamera::Projection::Orthographic);
				view = glm::inverse(transform);
				projection = camera.GetProjection();
			}
			else
			{
				ImGuizmo::SetOrthographic(false);
				view = m_EditorCamera.GetView();
				projection = m_EditorCamera.GetProjection();
			}

			glm::mat4 transform = m_ActiveScene->GetWorldSpaceTransformMatrix(m_SelectetEntity);

			float snapVal = 0.0f;
			if (Input::IsKeyDown(KeyCode::LeftShift))
			{
				switch (m_CurrentOperation)
				{
					case GizmoOperaton::Translate: snapVal = m_TranslationSnap; break;
					case GizmoOperaton::Rotate:    snapVal = m_RotationSnap; break;
					case GizmoOperaton::Scale:     snapVal = m_ScaleSnap; break;
				}
			}

			float snap[3] = { snapVal, snapVal, snapVal };
			glm::mat4 delta;
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), (ImGuizmo::OPERATION)m_CurrentOperation, ImGuizmo::LOCAL, glm::value_ptr(transform), glm::value_ptr(delta), snap);

			if (!Input::IsKeyDown(KeyCode::LeftAlt) && ImGuizmo::IsUsing())
			{
				glm::mat4 localTransform = transform;
				m_ActiveScene->ConvertToLocaSpace(m_SelectetEntity, localTransform);
				auto& tf = m_SelectetEntity.Transform();

				glm::vec3 translation, rotation, scale;
				if (Math::DecomposeTransform(localTransform, translation, rotation, scale))
				{
					glm::vec3 deltaRotation = rotation - tf.Rotation;
					tf.Translation = translation;
					tf.Rotation += deltaRotation;
					tf.Scale = scale;
				}
			}
		}
	}

	void EditorLayer::UI_EditorCamera()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowEditorCameraControlls)
		{
			if (ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls))
			{
				ImGuiID controlsID = UI::GenerateID();
				UI::BeginControls(controlsID);

				//UI::DragFloat("Position", m_EditorCamera.GetPosition());

				auto focuspoint = m_EditorCamera.GetFocusPoint();
				if (UI::Control("FocusPoint", focuspoint))
					m_EditorCamera.SetFocusPoint(focuspoint);

				glm::vec2 py = { m_EditorCamera.GetPitch(), m_EditorCamera.GetYaw() };
				if (UI::Control("Orientation", py))
				{
					m_EditorCamera.SetPicht(py.x);
					m_EditorCamera.SetYaw(py.y);
				}

				float distance = m_EditorCamera.GetDistance();
				if (UI::Control("Distance", distance, 10))
					if (distance >= 0.25f)
						m_EditorCamera.SetDistance(distance);

				float nearClip = m_EditorCamera.GetNearClip();
				if (UI::Control("near", nearClip))
					m_EditorCamera.SetNearClip(nearClip);
				
				float farClip = m_EditorCamera.GetFarClip();
				if (UI::Control("Far", farClip))
					m_EditorCamera.SetFarClip(farClip);

				UI::EndControls();
				ImGui::Separator();
				UI::BeginControls(controlsID);

				float flySpeed = m_EditorCamera.GetFlySpeed();
				if (UI::Control("Movement Speed", flySpeed))
					m_EditorCamera.SetFlySpeed(flySpeed);

				float rotationSpeed = m_EditorCamera.GetRotationSpeed();
				if (UI::Control("Rotation Speed", rotationSpeed))
					m_EditorCamera.SetRotationSpeed(rotationSpeed);

				float speedup = m_EditorCamera.GetSpeedup();
				if (UI::Control("Speedup", speedup))
					m_EditorCamera.SetSpeedup(speedup);

				float fov = m_EditorCamera.GetFOV();
				if (UI::Control("FOV", fov))
					m_EditorCamera.SetFOV(fov);

				UI::EndControls();

				if (ImGui::Button("Reset"))
				{
					m_EditorCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
					m_EditorCamera.SetPicht(0.0f);
					m_EditorCamera.SetYaw(0.0f);
					m_EditorCamera.SetDistance(10.0f);
				}

				UI::EndControls();
			}
			ImGui::End();
		}
	}

	void EditorLayer::UI_DragDrop()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_SceneState != SceneState::Edit)
			return;

		if (!ImGui::BeginDragDropTarget())
			return;

		// Asset
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET");
			if (payload)
			{
				AssetHandle handle = UI::GetPayloadDataAs<AssetHandle>(payload);
				const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(handle);
				if (metadata.IsValid())
				{
					switch (metadata.Type)
					{
						case AssetType::Scene:
						{
							LoadScene(handle);
							break;
						}
						case AssetType::Texture:
						{
							Entity entity = m_ActiveScene->CreateEntity();
							auto& sr = entity.AddComponent<SpriteRendererComponent>();
							sr.TextureHandle = handle;
							SelectEntity(entity);
							break;
						}
						case AssetType::TextureSource:
						{
							m_TextureAssetCreateData.Clear();
							auto sourcePath = Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata);
							m_TextureAssetCreateData.TextureSourcePath = sourcePath.string();
							m_TextureAssetCreateData.TextureFileName = sourcePath.stem().string();
							m_TextureAssetCreateData.OpenPopup = true;
							m_TextureAssetCreateData.CreateEntityAfterCreation = true;
							break;
						}
						case AssetType::Mesh:
						{
							Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(metadata.Handle);
							if (mesh)
							{
								InstantiateMesh(mesh, true);
							}
							break;
						}
						case AssetType::MeshSource:
						{
							m_CreateMeshAssetData = {};
							m_CreateMeshAssetData.Show = true;
							m_CreateMeshAssetData.MeshSource = handle;
							m_CreateMeshAssetData.DestinationPath = FileSystem::CreatePathString({}, FileSystem::GetStemString(metadata.FilePath), ".skmesh");
							m_CreateMeshAssetData.ParentDirectory = "Assets/Meshes/";
						}
					}
				}

			}
		}

		// ASSET_FILEPATH
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_FILEPATH");
			if (payload)
			{
				SK_DEBUG_BREAK_CONDITIONAL(s_Break);

				// What path type is ASSET_FILEPATH?

				std::filesystem::path filePath = std::wstring((const wchar_t*)payload->Data, payload->DataSize / sizeof(wchar_t));
				filePath = Project::GetActive()->GetAbsolute(filePath);
				const auto extention = filePath.extension();
					
				if (extention == L".skscene")
				{
					AssetHandle handle = Project::GetActiveEditorAssetManager()->ImportAsset(filePath);
					LoadScene(handle);
				}
				else if (extention == "L.sktex")
				{
					AssetHandle handle = Project::GetActiveEditorAssetManager()->ImportAsset(filePath);
					Entity newEntity = CreateEntity();
					auto& sr = newEntity.AddComponent<SpriteRendererComponent>();
					sr.TextureHandle = handle;
					SelectEntity(newEntity);
				}
				else if (extention == L".png")
				{
					m_TextureAssetCreateData.Clear();
					m_TextureAssetCreateData.TextureSourcePath = filePath.string();
					m_TextureAssetCreateData.OpenPopup = true;
					m_TextureAssetCreateData.CreateEntityAfterCreation = true;
				}
			}
		}

		ImGui::EndDragDropTarget();
	}

	void EditorLayer::UI_ToolBar()
	{
		SK_PROFILE_FUNCTION();
		
		constexpr ImGuiWindowFlags falgs = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

		const ImGuiStyle& style = ImGui::GetStyle();
		
		UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, { 0, 2 });
		UI::ScopedStyle innerItemSpacing(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
		UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, { 2, style.ItemSpacing.y });
		UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
		auto colHovered = style.Colors[ImGuiCol_ButtonHovered];
		auto colActive = style.Colors[ImGuiCol_ButtonActive];
		colHovered.w = colActive.w = 0.5f;

		UI::ScopedColor button(ImGuiCol_Button, { 0, 0, 0, 0 });
		UI::ScopedColor buttonHovered(ImGuiCol_ButtonHovered, colHovered);
		UI::ScopedColor buttonActive(ImGuiCol_ButtonActive, colActive);
		ImGui::Begin("##ViewPortToolBar", nullptr, falgs);

		const float height = ImGui::GetContentRegionAvail().y;
		const ImVec2 size = { height, height };

		const float windowContentRegionWith = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		ImGui::SetCursorPosX(windowContentRegionWith * 0.5f - (size.x * 3.0f * 0.5f) - (style.ItemSpacing.x * 3.0f));

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				if (ImGui::ImageButton("Play", Icons::PlayIcon->GetViewID(), size))
					SubmitOnScenePlay();

				ImGui::SameLine();

				if (ImGui::ImageButton("Simulate", Icons::SimulateIcon->GetViewID(), size))
					SubmitOnSimulationPlay();

				ImGui::SameLine();
				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, true);
					ImGui::ImageButton("Step Disabled", Icons::StepIcon->GetViewID(), size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, { 0.5f, 0.5f, 0.5f, 1.0f });
				}

				break;
			}
			case SceneState::Play:
			{
				if (ImGui::ImageButton("Stop", Icons::StopIcon->GetViewID(), size))
					SubmitOnSceneStop();

				ImGui::SameLine();

				Ref<Texture2D> pausePlayIcon = m_ActiveScene->IsPaused() ? Icons::PlayIcon : Icons::PauseIcon;
				if (ImGui::ImageButton("PausePlay", pausePlayIcon->GetViewID(), size))
					SubmitSetScenePaused(!m_ActiveScene->IsPaused());

				ImGui::SameLine();

				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, !m_ActiveScene->IsPaused());
					const ImVec4 tintColor = m_ActiveScene->IsPaused() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
					if (ImGui::ImageButton("Step", Icons::StepIcon->GetViewID(), size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, tintColor))
						SubmitStepScene(1);
				}

				break;
			}
			case SceneState::Simulate:
			{
				if (ImGui::ImageButton("StopIcon", Icons::StopIcon->GetViewID(), size))
					SubmitOnSimulationStop();

				ImGui::SameLine();

				Ref<Texture2D> pausePlayIcon = m_ActiveScene->IsPaused() ? Icons::PlayIcon : Icons::PauseIcon;
				if (ImGui::ImageButton("PausePlayIcon", pausePlayIcon->GetViewID(), size))
					SubmitSetScenePaused(!m_ActiveScene->IsPaused());

				ImGui::SameLine();

				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, !m_ActiveScene->IsPaused());
					const ImVec4 tintColor = m_ActiveScene->IsPaused() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
					if (ImGui::ImageButton("Step", Icons::StepIcon->GetViewID(), size, {0, 0}, {1, 1}, {0, 0, 0, 0}, tintColor))
						SubmitStepScene(1);
				}

				break;
			}
		}

		ImGui::End();
	}

	void EditorLayer::UI_ImportTexture()
	{
		SK_PROFILE_FUNCTION();

		if (m_TextureAssetCreateData.OpenPopup)
		{
			ImGui::OpenPopup("Import Texture");
			m_TextureAssetCreateData.OpenPopup = false;
		}

		if (ImGui::BeginPopupModal("Import Texture"))
		{
			ImGui::Text("Not implemented. Will be replaced with Texture/TextureSource rework");
#if 0
			UI::Text("Input FileName");
			UI::Text(fmt::format("Parent Path: {}/Texture", Project::GetActiveAssetsDirectory()));
			ImGui::InputText("##FileName", &m_TextureAssetCreateData.TextureFileName);

			if (ImGui::Button("Import"))
			{
				EditorLayer* editor = this;
				Application::Get().SubmitToMainThread([editor]()
				{
					std::string directory = String::ToNarrow(fmt::format(L"{}/Textures", Project::GetActiveAssetsDirectory().native()));
					std::string fileName = std::filesystem::path(editor->m_TextureAssetCreateData.TextureFileName).replace_extension(".sktex").string();

					const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(editor->m_TextureAssetCreateData.TextureSourcePath);
					Ref<TextureSource> textureSource = AssetManager::GetAsset<TextureSource>(metadata.Handle);
					Ref<Texture2D> texture = Project::GetActiveEditorAssetManager()->CreateAsset<Texture2D>(directory, fileName, TextureSpecification{}, textureSource);

					if (editor->m_TextureAssetCreateData.CreateEntityAfterCreation)
					{
						Entity entity = editor->m_ActiveScene->CreateEntity();
						auto& sr = entity.AddComponent<SpriteRendererComponent>();
						sr.TextureHandle = texture->Handle;
						editor->SelectEntity(entity);
					}
				});
				ImGui::CloseCurrentPopup();
			}
#endif

			ImGui::SameLine();

			if (ImGui::Button("Cancle"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	bool EditorLayer::UI_MousePicking()
	{
		SK_PROFILE_FUNCTION();

		if (ImGuizmo::IsUsing())
			return false;

		auto [mx, my] = ImGui::GetMousePos();
		auto [wx, wy] = m_ViewportPos;
		int x = (int)(mx - wx);
		int y = (int)(my - wy);

		const auto& specs = m_MousePickingImage->GetSpecification();
		int width = (int)specs.Width;
		int height = (int)specs.Height;
		if (x >= 0 && x < (int)width && y >= 0 && y < (int)height)
		{
			const bool selectEntity = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::IsKeyDown(KeyCode::LeftAlt) && m_ViewportHovered;
			uint32_t hoverdEntity = (uint32_t)-1;

			if (selectEntity)
			{
				Renderer::RT_CopyImage(Renderer::GetCommandBuffer(), m_SceneRenderer->GetIDImage(), m_MousePickingImage);
				if (!m_MousePickingImage->RT_ReadPixel(x, y, hoverdEntity))
					return false;

				if (hoverdEntity == (uint32_t)-1)
				{
					SelectEntity(Entity{});
					return true;
				}

				Entity entity = { (entt::entity)hoverdEntity, m_ActiveScene };
				if (entity)
				{
					if (Input::IsKeyDown(KeyCode::LeftShift))
					{
						while (entity.HasParent())
						{
							entity = entity.Parent();
						}
					}

					SelectEntity(entity);
				}
			}
		}
		return true;
	}

	void EditorLayer::UI_LogSettings()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ShowLogSettings)
			return;

		if (ImGui::Begin("Log", &m_ShowLogSettings))
		{
			auto& tags = Log::GetTags();
			ImGui::Separator();
			for (auto& [tag, data] : tags)
			{
				UI::BeginControlsGrid();
				UI::Property("Tag", tag);
				UI::ControlCombo("Level", (uint16_t&)data.Level, LogLevelStrings, (uint16_t)std::size(LogLevelStrings));
				UI::Control("Enabled", data.Enabled);
				UI::EndControls();
				ImGui::Separator();
			}
		}
		ImGui::End();
	}

	void EditorLayer::UI_OpenProjectModal()
	{
		SK_PROFILE_FUNCTION();

		if (!m_OpenProjectModal.Show)
			return;

		if (m_OpenProjectModal.OpenPopup)
		{
			ImGui::OpenPopup("Open Project");
			m_OpenProjectModal.OpenPopup = false;
		}

		if (ImGui::BeginPopupModal("Open Project"))
		{
			ImGui::Text("Do you want to Open this Project?");
			UI::Text(m_OpenProjectModal.ProjectFile);

			const ImGuiStyle& style = ImGui::GetStyle();

			ImVec2 openTextSize = ImGui::CalcTextSize("Open");
			ImVec2 closeTextSize = ImGui::CalcTextSize("Close");
			
			ImVec2 openButtonSize = { openTextSize.x + style.FramePadding.x * 2.0f, openTextSize.y + style.FramePadding.y * 2.0f };
			ImVec2 closeButtonSize = { closeTextSize.x + style.FramePadding.x * 2.0f, closeTextSize.y + style.FramePadding.y * 2.0f };

			ImVec2 contentRegion = ImGui::GetContentRegionMax();

			ImVec2 openButtonPos = {
				contentRegion.x - closeButtonSize.x - style.ItemSpacing.x - openButtonSize.x,
				contentRegion.y - openButtonSize.y
			};

			ImGui::SetCursorPos(openButtonPos);
			if (ImGui::Button("Open"))
			{
				OpenProject(m_OpenProjectModal.ProjectFile);
				m_OpenProjectModal.Reset();
			}

			ImGui::SameLine();
			if (ImGui::Button("Close"))
			{
				m_OpenProjectModal.Reset();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::UI_ImportAsset()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ImportAssetData.Show)
			return;

		if (ImGui::Begin("Import Asset"))
		{
			const ImGuiStyle& style = ImGui::GetStyle();

			UI::TextF("Importing Asset from {}", m_ImportAssetData.SourcePath);

			char buffer[MAX_PATH];
			strcpy_s(buffer, m_ImportAssetData.DestinationPath.c_str());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::CalcItemSizeFromText("...").x - style.ItemSpacing.x);

			bool invalidInput;
			if (UI::InputPath("##destPath", buffer, MAX_PATH, invalidInput))
				m_ImportAssetData.DestinationPath = buffer;

			if (invalidInput)
			{
				m_ImportAssetData.Error = "Filepaths aren't allowed to containe any of the folloing character!\n:*?\"<>|";
				m_ImportAssetData.ShowError = true;
			}

			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				auto path = Platform::SaveFileDialog(L"", 1, Project::GetActiveAssetsDirectory(), true, false);
				path.replace_extension();
				m_ImportAssetData.DestinationPath = std::filesystem::relative(path, Project::GetActiveAssetsDirectory()).generic_string();
			}

			if (m_ImportAssetData.ShowError)
			{
				UI::ScopedColor errorText(ImGuiCol_Text, Theme::Colors::TextInvalidInput);
				UI::Text(m_ImportAssetData.Error);
				m_ImportAssetData.ShowErrorTimer += m_TimeStep;
				if (m_ImportAssetData.ShowErrorTimer >= m_ImportAssetData.ShowErrorDuration)
				{
					m_ImportAssetData.ShowError = false;
					m_ImportAssetData.ShowErrorTimer = 0.0f;
				}
			}

			ImVec2 importTextSize = ImGui::CalcTextSize("Import");
			ImVec2 cancleTextSize = ImGui::CalcTextSize("Cancle");

			ImVec2 importButtonSize = { importTextSize.x + style.FramePadding.x * 2.0f, importTextSize.y + style.FramePadding.y * 2.0f };
			ImVec2 cancleButtonSize = { cancleTextSize.x + style.FramePadding.x * 2.0f, cancleTextSize.y + style.FramePadding.y * 2.0f };

			ImVec2 contentRegion = ImGui::GetContentRegionMax();

			ImVec2 importButtonPos = {
				contentRegion.x - cancleButtonSize.x - style.ItemSpacing.x - importButtonSize.x,
				contentRegion.y - importButtonSize.y
			};

			ImGui::SetCursorPos(importButtonPos);
			if (ImGui::Button("Import"))
			{
				std::filesystem::path destination = Project::GetActiveAssetsDirectory() / m_ImportAssetData.DestinationPath;
				std::filesystem::path source = m_ImportAssetData.SourcePath;

				bool tryImport = true;

				if (destination.native().find(L"..") != std::wstring::npos)
				{
					m_ImportAssetData.Error = "Invalid Destination Path";
					m_ImportAssetData.ShowError = true;
					tryImport = false;
				}

				if (source.native().find(L"..") != std::wstring::npos)
				{
					m_ImportAssetData.Error = "Invalid Source Path";
					m_ImportAssetData.ShowError = true;
					tryImport = false;
				}

				if (tryImport)
				{
					destination.replace_extension(source.extension());

#if 0
					auto destinationDirectory = destination.parent_path();
					if (!FileSystem::Exists(destinationDirectory))
						FileSystem::CreateDirectories(destinationDirectory);
#endif

					std::string errorMsg;
					if (!FileSystem::CopyFile(source, destination, errorMsg))
					{
						m_ImportAssetData.Error = errorMsg;
						m_ImportAssetData.ShowError = true;
					}
					else
					{
						Project::GetActiveEditorAssetManager()->ImportAsset(destination);
						m_ImportAssetData = {};
					}
				}
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Cancle"))
			{
				m_ImportAssetData = {};
			}

			ImGui::End();
		}
	}

	void EditorLayer::UI_CreateMeshAsset()
	{
		if (!m_CreateMeshAssetData.Show)
			return;

		if (ImGui::Begin("Create Mesh Asset"))
		{
			ImGui::TextWrapped("This file cannot be used directly, a Shark metafile must first be created to interpret the contents of the file.");
			ImGui::Separator();

			UI::BeginControlsGrid();
			std::string label = fmt::format("path: {}", m_CreateMeshAssetData.ParentDirectory);
			UI::Control(label, m_CreateMeshAssetData.DestinationPath);
			UI::EndControlsGrid();

			if (ImGui::Button("Create"))
			{
				std::filesystem::path fullPath = m_CreateMeshAssetData.ParentDirectory / m_CreateMeshAssetData.DestinationPath;
				FileSystem::ReplaceExtension(fullPath, ".skmesh");
				
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(m_CreateMeshAssetData.MeshSource);
				Ref<Mesh> mesh = Project::GetActiveEditorAssetManager()->CreateAsset<Mesh>(fullPath, meshSource);
				InstantiateMesh(mesh, true);
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancle"))
			{
				m_CreateMeshAssetData = {};
			}

		}
		ImGui::End();

	}

	void EditorLayer::DebugRender()
	{
		SK_PROFILE_FUNCTION();

		//m_DebugRenderer->SetRenderTarget(m_SceneRenderer->GetExternalCompositFrameBuffer());
		m_DebugRenderer->BeginScene(GetActiveViewProjection());

		if (m_ShowColliders)
		{
			{
				auto view = m_ActiveScene->GetAllEntitysWith<BoxCollider2DComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, m_ActiveScene };
					auto& collider = view.get<BoxCollider2DComponent>(entityID);
					auto& tf = entity.Transform();

					glm::mat4 transform =
						m_ActiveScene->GetWorldSpaceTransformMatrix(entity) *
						glm::translate(glm::vec3(collider.Offset, 0)) *
						glm::rotate(collider.Rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
						glm::scale(glm::vec3(collider.Size * 2.0f, 1.0f));
					
					m_DebugRenderer->DrawRect(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
				}
			}

			{
				auto view = m_ActiveScene->GetAllEntitysWith<CircleCollider2DComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, m_ActiveScene };
					auto& collider = view.get<CircleCollider2DComponent>(entityID);
					auto& tf = entity.Transform();

					glm::mat4 transform =
						m_ActiveScene->GetWorldSpaceTransformMatrix(entity) *
						glm::translate(glm::vec3(collider.Offset, 0)) *
						glm::rotate(collider.Rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
						glm::scale(glm::vec3(collider.Radius, collider.Radius, 1.0f));

					m_DebugRenderer->DrawCircle(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
				}
			}
		}

		if (m_ShowLightRadius)
		{
			if (m_SelectetEntity && m_SelectetEntity.AllOf<PointLightComponent>())
			{
				const auto& plc = m_SelectetEntity.GetComponent<PointLightComponent>();
				
				glm::vec3 center = m_SelectetEntity.Transform().Translation;
				glm::vec4 lightRadiusColor = { 0.1f, 0.3f, 0.9f, 1.0f };
				float rotation = glm::radians(90.0f);

				m_DebugRenderer->DrawCircle(center, { 0.0f, 0.0f, 0.0f }, plc.Radius, lightRadiusColor);
				m_DebugRenderer->DrawCircle(center, { 0.0f, rotation, 0.0f }, plc.Radius, lightRadiusColor);
				m_DebugRenderer->DrawCircle(center, { rotation, 0.0f, 0.0f }, plc.Radius, lightRadiusColor);
			}
		}

		m_DebugRenderer->EndScene();
	}

	Entity EditorLayer::CreateEntity(const std::string& name)
	{
		return m_ActiveScene->CreateEntity(name);
	}

	void EditorLayer::DeleteEntity(Entity entity)
	{
		if (entity.GetUUID() == m_ActiveScene->GetActiveCameraUUID())
			m_ActiveScene->SetActiveCamera(UUID());

		m_ActiveScene->DestroyEntity(entity);
		SelectEntity(Entity{});
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		m_SelectetEntity = entity;
		m_PanelManager->Get<SceneHirachyPanel>(SCENE_HIRACHY_PANEL_ID)->SetSelectedEntity(entity);
	}

	glm::mat4 EditorLayer::GetActiveViewProjection() const
	{
		SK_PROFILE_FUNCTION();

		if (m_SceneState == SceneState::Play)
		{
			Entity cameraEntity = m_ActiveScene->GetActiveCameraEntity();
			auto& camera = cameraEntity.GetComponent<CameraComponent>();
			auto& tf = cameraEntity.Transform();
			return camera.GetProjection() * glm::inverse(m_ActiveScene->GetWorldSpaceTransformMatrix(cameraEntity));
		}

		return m_EditorCamera.GetViewProjection();
	}

	void EditorLayer::NewScene(const std::string& name)
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		AssetHandle handle = AssetManager::CreateMemoryAsset<Scene>(name);
		auto newScene = AssetManager::GetAsset<Scene>(handle);
		m_WorkScene = newScene;
		SetActiveScene(newScene);
		m_EditorCamera.SetView(glm::vec3(0.0f), 10.0f, 0.0f, 0.0f);
	}

	bool EditorLayer::LoadScene(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(filePath);
		if (!metadata.IsValid())
			return false;

		return LoadScene(metadata.Handle);
	}

	bool EditorLayer::LoadScene(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		Ref<Scene> scene = AssetManager::GetAsset<Scene>(handle);
		if (scene)
		{
			m_WorkScene = scene;
			SetActiveScene(scene);
			m_EditorCamera.SetView(glm::vec3(0.0f), 10.0f, 0.0f, 0.0f);
			return true;
		}
		return false;
	}

	bool EditorLayer::SaveScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_WorkScene);

		if (AssetManager::IsMemoryAsset(m_WorkScene->Handle))
			return SaveSceneAs();

		return Project::GetActiveEditorAssetManager()->SaveAsset(m_WorkScene->Handle);
	}

	bool EditorLayer::SaveSceneAs()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_WorkScene);

		auto filePath = Platform::SaveFileDialog(L"|*.*|Scene|*.skscene", 2, Project::GetActiveAssetsDirectory(), true);
		if (filePath.empty())
			return false;

		SK_CORE_ASSERT(filePath.extension() == L".skscene");
		if (filePath.extension() != L".skscene")
			return false;

		if (AssetManager::IsMemoryAsset(m_WorkScene->Handle))
		{
			std::string directoryPath = Project::GetActiveEditorAssetManager()->MakeRelativePathString(filePath.parent_path());
			std::string fileName = filePath.filename().string();
			return Project::GetActiveEditorAssetManager()->ImportMemoryAsset(m_WorkScene->Handle, directoryPath, fileName);
		}

		std::string directoryPath = Project::GetActiveEditorAssetManager()->MakeRelativePathString(filePath.parent_path());
		std::string fileName = filePath.filename().string();
		Ref<Scene> newScene = Project::GetActiveEditorAssetManager()->CreateAsset<Scene>(directoryPath, fileName);
		m_WorkScene->CopyTo(newScene);
		return Project::GetActiveEditorAssetManager()->SaveAsset(newScene->Handle);
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		m_RenderGizmo = false;

		SetActiveScene(Scene::Copy(m_WorkScene));
		m_PanelManager->OnScenePlay();
		m_ActiveScene->OnScenePlay();
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_RenderGizmo = true;

		m_ActiveScene->OnSceneStop();
		m_PanelManager->OnSceneStop();
		SetActiveScene(m_WorkScene);
	}

	void EditorLayer::OnSimulationPlay()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Simulate;
		m_RenderGizmo = false;

		SetActiveScene(Scene::Copy(m_WorkScene));
		m_ActiveScene->OnSimulationPlay();
	}

	void EditorLayer::OnSimulationStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_RenderGizmo = true;

		m_ActiveScene->OnSimulationStop();
		m_PanelManager->OnSceneStop();
		SetActiveScene(m_WorkScene);
	}

	void EditorLayer::SubmitOnScenePlay()
	{
		Application::Get().SubmitToMainThread([this]() { OnScenePlay(); });
	}

	void EditorLayer::SubmitOnSceneStop()
	{
		Application::Get().SubmitToMainThread([this]() { OnSceneStop(); });
	}

	void EditorLayer::SubmitOnSimulationPlay()
	{
		Application::Get().SubmitToMainThread([this]() { OnSimulationPlay(); });
	}

	void EditorLayer::SubmitOnSimulationStop()
	{
		Application::Get().SubmitToMainThread([this]() { OnSimulationStop(); });
	}

	void EditorLayer::SubmitSetScenePaused(bool paused)
	{
		Application::Get().SubmitToMainThread([paused, scene = m_ActiveScene]() { scene->SetPaused(paused); });
	}

	void EditorLayer::SubmitStepScene(uint32_t frames)
	{
		Application::Get().SubmitToMainThread([frames, scene = m_ActiveScene]() { scene->Step(frames); });
	}

	void EditorLayer::SetActiveScene(Ref<Scene> scene)
	{
		SK_PROFILE_FUNCTION();

		if (m_ActiveScene && AssetManager::IsMemoryAsset(m_ActiveScene->Handle))
			Project::GetActiveEditorAssetManager()->UnloadAsset(m_ActiveScene->Handle);

		if (scene && m_SelectetEntity)
			SelectEntity(scene->TryGetEntityByUUID(m_SelectetEntity.GetUUID()));

		if (scene)
		{
			scene->IsEditorScene(m_SceneState != SceneState::Play);
			scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}

		m_ActiveScene = scene;
		m_SceneRenderer->SetScene(scene);
		m_PanelManager->SetContext(scene);
		UpdateWindowTitle();
	}

	void EditorLayer::OpenProject()
	{
		SK_PROFILE_FUNCTION();

		auto filePath = Platform::OpenFileDialog(L"|*.*|Project|*.skproj", 2);
		if (!filePath.empty())
			OpenProject(filePath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("Opening Project [{}]", filePath);

		auto project = Project::LoadEditor(filePath);
		if (project)
		{
			if (Project::GetActive())
				CloseProject();

			Project::SetActive(project);

			ScriptEngine::LoadAssemblies(project->GetConfig().ScriptModulePath);
			m_PanelManager->OnProjectChanged(project);

			if (!LoadScene(project->GetConfig().StartupScene))
				NewScene("Empty Fallback Scene");
		}
	}

	void EditorLayer::CloseProject()
	{
		SK_PROFILE_FUNCTION();

		Project::SaveActive();
		Project::GetActive()->GetEditorAssetManager()->SerializeImportedAssets();

		Weak sceneWeak = m_WorkScene;
		Weak projectWeak = Project::GetActive();

		SK_CORE_INFO("Closing Project");

		m_ActiveScene = nullptr;
		m_SceneRenderer->SetScene(nullptr);
		SetActiveScene(nullptr);

		ScriptEngine::UnloadAssemblies();
		m_WorkScene = nullptr;

		m_PanelManager->OnProjectChanged(nullptr);
		Project::SetActive(nullptr);

		SK_CORE_ASSERT(sceneWeak.Expired());
		SK_CORE_ASSERT(projectWeak.Expired());
	}
	
	Ref<Project> EditorLayer::CreateProject(const std::filesystem::path& projectDirectory)
	{
		// Create Directory

		std::string name = projectDirectory.filename().string();
		if (std::filesystem::exists(projectDirectory))
		{
			SK_CORE_ERROR("Directory already exists!");
			return nullptr;
		}

		std::filesystem::create_directories(projectDirectory);
		auto project = Project::Create(projectDirectory, name);

		std::filesystem::create_directory(projectDirectory / "Binaries");
		std::filesystem::create_directory(projectDirectory / "Assets");
		std::filesystem::create_directory(projectDirectory / "Assets/Scenes");
		std::filesystem::create_directory(projectDirectory / "Assets/Scripts");
		std::filesystem::create_directory(projectDirectory / "Assets/Scripts/Source");
		std::filesystem::create_directory(projectDirectory / "Assets/Textures");
		std::filesystem::create_directory(projectDirectory / "Assets/TextureSources");

		CreateProjectPremakeFile(project);
		std::filesystem::copy_file("Resources/Project/Setup.bat", fmt::format("{0}/Setup.bat", project->GetDirectory()));
		std::filesystem::copy("Resources/Project/Premake", project->GetDirectory() / "Premake", std::filesystem::copy_options::recursive);

		ProjectSerializer serializer(project);
		serializer.Serialize(fmt::format("{0}/{1}.skproj", project->GetDirectory(), project->GetConfig().Name));

		return project;
	}

	void EditorLayer::CreateProjectPremakeFile(Ref<Project> project)
	{
		const std::string projectNameToken = "%PROJECT_NAME%";

		std::string premakeTemplate = FileSystem::ReadString("Resources/Project/PremakeFileTemplate.lua");
		String::Replace(premakeTemplate, projectNameToken,  project->GetConfig().Name);

		auto premakeFilePath = fmt::format("{0}/premake5.lua", project->GetDirectory());
		FileSystem::WriteString(premakeFilePath, premakeTemplate);
	}

	void EditorLayer::RunScriptSetup()
	{
		ExecuteSpecs specs;
		specs.Target = fmt::format(L"{}/Premake/premake5.exe", Project::GetActiveDirectory());
		// vs2022 dosn't work for some reason but vs2019 still generates vs2022 solution
		auto sharkDir = std::filesystem::current_path().parent_path();
		specs.Params = L"vs2019";
		specs.WaitUntilFinished = true;
		specs.WorkingDirectory = Project::GetActiveDirectory();
		specs.InterhitConsole = true;
		Platform::Execute(specs);
	}

	void EditorLayer::OpenIDE()
	{
		auto solutionPath = fmt::format("{}/{}.sln", Project::GetActiveDirectory(), Project::GetActive()->GetConfig().Name);
		Platform::Execute(ExectueVerb::Open, solutionPath);
	}

	void EditorLayer::AssembliesReloadedHook()
	{
		if (!m_ActiveScene)
			return;

		auto view = m_ActiveScene->GetAllEntitysWith<ScriptComponent>();
		for (auto ent : view)
		{
			Entity entity{ ent, m_ActiveScene };
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(scriptComponent.ScriptName);
			scriptComponent.ClassID = klass ? klass->GetID() : 0;
		}
	}

	void EditorLayer::UpdateWindowTitle()
	{
		// Scene File name (Scene Name) - Editor Name - Platform - Renderer

		std::string sceneFilePath;
		std::string sceneName;
		if (m_ActiveScene)
		{
			auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(m_ActiveScene);
			sceneFilePath = metadata.FilePath.filename().string();
			sceneName = m_ActiveScene->GetName();
		}

		std::string title = fmt::format("{} ({}) - SharkFin - {} {} ({}) - {}", sceneFilePath, sceneName, Platform::GetPlatformName(), Platform::GetArchitecture(), Platform::GetConfiguration(), ToStringView(RendererAPI::GetCurrentAPI()));
		Application::Get().GetWindow().SetTitle(title);
	}

	Entity EditorLayer::InstantiateMesh(Ref<Mesh> mesh, bool select)
	{
		SK_PROFILE_FUNCTION();

		Ref<MeshSource> source = mesh->GetMeshSource();
		Entity rootEntity = m_ActiveScene->CreateEntity();
		InstantiateMeshNode(mesh, source->GetRootNode(), Entity{}, rootEntity);
		
		if (select)
		{
			SelectEntity(rootEntity);
		}

		return rootEntity;
	}

	void EditorLayer::InstantiateMeshNode(Ref<Mesh> mesh, const MeshNode& node, Entity parent, Entity entity)
	{
		SK_PROFILE_FUNCTION();

		Ref<MeshSource> source = mesh->GetMeshSource();

		if (!entity)
			entity = m_ActiveScene->CreateChildEntity(parent);
		entity.GetName() = node.Name;

		entity.Transform().SetTransform(node.LocalTransform);
		if (node.Submeshes.size() == 1)
		{
			auto& meshComp = entity.AddComponent<MeshRendererComponent>();
			meshComp.MeshHandle = mesh->Handle;
			meshComp.SubmeshIndex = node.Submeshes[0];
		}
		else if (node.Submeshes.size() > 1)
		{
			Entity container = m_ActiveScene->CreateChildEntity(entity, fmt::format("{} (Submesh Container)", node.Name));
			for (uint32_t submeshIndex = 0; submeshIndex < node.Submeshes.size(); submeshIndex++)
			{
				Entity submeshEntity = m_ActiveScene->CreateChildEntity(container, fmt::format("{}_{} (Submesh)", node.Name, submeshIndex));
				auto& meshComp = submeshEntity.AddComponent<MeshRendererComponent>();
				meshComp.MeshHandle = mesh->Handle;
				meshComp.SubmeshIndex = submeshIndex;
			}
		}

		for (const auto& childNodeIndex : node.Children)
		{
			const MeshNode& childNode = source->GetNodes()[childNodeIndex];
			InstantiateMeshNode(mesh, childNode, entity);
		}
	}

	void EditorLayer::VerifyEditorTexture(const std::filesystem::path& assetPath)
	{
		VerifyEditorTexture(assetPath, FileSystem::ChangeExtension(assetPath, ".png"));
	}

	void EditorLayer::VerifyEditorTexture(const std::filesystem::path& assetPath, const std::filesystem::path& sourcePath)
	{
		Ref<EditorAssetManager> assetManager = Project::GetActiveEditorAssetManager();

		if (assetManager->HasEditorAsset(assetPath))
			return;

		if (FileSystem::Exists(assetPath))
		{
			assetManager->AddEditorAsset(assetPath);
			return;
		}

		AssetHandle sourceHandle = assetManager->GetEditorAsset(sourcePath);
		if (sourceHandle == AssetHandle::Invalid)
		{
			SK_CORE_ERROR("Missing Texture! {}", sourcePath);
			return;
		}

		Ref<TextureSource> textureSource = AssetManager::GetAsset<TextureSource>(sourceHandle);
		Ref<Texture2D> texture = Texture2D::Create(textureSource);
		assetManager->AddEditorAsset(texture, assetPath);
		assetManager->SaveAsset(texture->Handle);
	}

	void EditorLayer::VerifyAllEditorAssets()
	{
		VerifyEditorTexture("Resources/Textures/NoImagePlaceholder.sktex", "Resources/Textures/NoImagePlaceholder.png");
	}

}
