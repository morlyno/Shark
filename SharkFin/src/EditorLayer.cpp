#include "skfpch.h"
#include "EditorLayer.h"

#include "Shark/Core/Project.h"
#include <Shark/Scene/Components.h>
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"

#include "Shark/UI/UI.h"

#include "Shark/Editor/Icons.h"
#include "Shark/Editor/EditorConsole/EditorConsolePanel.h"
#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"
#include "Panels/PhysicsDebugPanel.h"
#include "Panels/ScriptEnginePanel.h"

#include "Shark/Editor/EditorSettings.h"
#include "Shark/Editor/EditorConsole/EditorConsolePanel.h"
#include "Shark/Editor/Icons.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/enttDebug.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "Panels/AssetsPanel.h"

#define SCENE_HIRACHY_ID "SceneHirachyPanel"
#define CONTENT_BROWSER_ID "ContentBrowserPanel"
#define TEXTURE_EDITOR_ID "TextureEditorPanel"
#define PHYSICS_DEBUG_ID "PhysicsDebugPanel"
#define ASSET_EDITOR_ID "AssetsEditorPanel"
#define EDITOR_CONSOLE_ID "EditorConsolePanel"
#define SCRIPT_ENGINE_ID "ScriptEnginePanel"
#define ASSETS_PANEL_ID "AssetsPanel"

namespace Shark {

	static bool s_ShowDemoWindow = false;

	EditorLayer::EditorLayer(const std::filesystem::path& startupProject)
		: Layer("EditorLayer"), m_StartupProject(startupProject)
	{
		SK_PROFILE_FUNCTION();
	}

	EditorLayer::~EditorLayer()
	{
		SK_PROFILE_FUNCTION();
	}

	void EditorLayer::OnAttach()
	{
		SK_PROFILE_FUNCTION();

		Icons::Init();

		EditorSettings::Init();
		m_PanelManager = Scope<PanelManager>::Create();

		auto sceneHirachy = m_PanelManager->AddPanel<SceneHirachyPanel>(SCENE_HIRACHY_ID, "Scene Hirachy", true);
		sceneHirachy->SetSelectionChangedCallback([this](Entity entity) { m_SelectetEntity = entity; });

		m_PanelManager->AddPanel<PhysicsDebugPanel>(PHYSICS_DEBUG_ID, "Pyhsics Debug", true);
		m_PanelManager->AddPanel<AssetEditorPanel>(ASSET_EDITOR_ID, "Assets Editor", false);
		m_PanelManager->AddPanel<EditorConsolePanel>(EDITOR_CONSOLE_ID, "Console", true);
		m_PanelManager->AddPanel<ContentBrowserPanel>(CONTENT_BROWSER_ID, "Content Browser", true);
		m_PanelManager->AddPanel<AssetsPanel>(ASSETS_PANEL_ID, "Assets", false);
		m_PanelManager->AddPanel<ScriptEnginePanel>(SCRIPT_ENGINE_ID, "Script Engine", false);

		m_SceneRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);
		m_CameraPreviewRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);
		m_DebugRenderer = Ref<Renderer2D>::Create(m_SceneRenderer->GetExternalCompositFrameBuffer());

		// Readable Mouse image for Mouse Picking
		ImageSpecification imageSpecs = m_SceneRenderer->GetIDImage()->GetSpecification();
		imageSpecs.Type = ImageType::Storage;
		m_MousePickingImage = Image2D::Create(imageSpecs, nullptr);

		// Load Project
		if (!m_StartupProject.empty())
			OpenProject(m_StartupProject);
		else
			SK_CORE_ASSERT(false, "No Startup Project!");

		FileSystem::SetCallback(std::bind(&EditorLayer::OnFileEvents, this, std::placeholders::_1));

		auto& app = Application::Get();
		if (app.GetSpecs().EnableImGui)
		{
			m_MainViewportID = ImHashStr("MainViewport");
			app.GetImGuiLayer().SetMainViewportID(m_MainViewportID);
		}
	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		CloseProject();

		m_PanelManager->Clear();

		Icons::Shutdown();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		Renderer::NewFrame();

		if (m_ActiveScene->Flags & AssetFlag::Unloaded)
		{
			Ref<Scene> scene = ResourceManager::GetAsset<Scene>(m_ActiveScene->Handle);
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
				m_CameraPreviewRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
				m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
				m_MousePickingImage->Resize(m_ViewportWidth, m_ViewportHeight);

				m_NeedsResize = false;
			}

			if ((m_ViewportHovered || m_ViewportFocused) && m_SceneState != SceneState::Play)
				m_EditorCamera.OnUpdate(ts);

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
				case SceneState::Pause:
				{
					if (m_UpdateNextFrame)
					{
						if (m_InitialSceneState == SceneState::Play)
							m_ActiveScene->OnUpdateRuntime(1.0f / 60.0f);
						else if (m_InitialSceneState == SceneState::Simulate)
							m_ActiveScene->OnUpdateSimulate(1.0f / 60.0f);

						m_UpdateNextFrame = false;
					}

					m_ActiveScene->OnRenderEditor(m_SceneRenderer, m_EditorCamera);
					break;
				}
			}

			RenderCameraPreview();
			DebugRender();
		}

		m_PanelManager->OnUpdate(ts);
	}

	void EditorLayer::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));

		if (event.Handled)
			return;

		if (m_ViewportHovered && m_SceneState != SceneState::Play)
			m_EditorCamera.OnEvent(event);

		m_PanelManager->OnEvent(event);
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		SK_PROFILE_FUNCTION();

		// Disable hot keys when the scene state is not Edit
		if (m_SceneState != SceneState::Edit)
			return false;

		if (event.IsRepeat())
			return false;

		const bool control = Input::IsKeyDown(KeyCode::LeftControl);
		const bool shift = Input::IsKeyDown(KeyCode::LeftShift);

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
				window.SetVSync(!window.IsVSync());
				return true;
			}

			// ImGuizmo
			case KeyCode::Q: { m_CurrentOperation = GizmoOperaton::None; return true; }
			case KeyCode::W: { m_CurrentOperation = GizmoOperaton::Translate; return true; }
			case KeyCode::E: { m_CurrentOperation = GizmoOperaton::Rotate; return true; }
			case KeyCode::R: { m_CurrentOperation = GizmoOperaton::Scale; return true; }
		}

		return false;
	}

	void EditorLayer::OnFileEvents(const std::vector<FileChangedData>& fileEvents)
	{
		m_PanelManager->GetPanel<ContentBrowserPanel>(CONTENT_BROWSER_ID)->OnFileEvents(fileEvents);

		// NOTE(moro): Resource Manager must be the last thing to recive file events
		ResourceManager::OnFileEvents(fileEvents);
	}

	void EditorLayer::OnFileClickedCallback(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		//const auto extension = filePath.extension();
		//if (extension == L".cs")
		//{
		//	//IDEUtils::OpenScript(fsPath);
		//	// open script file
		//	return;
		//}

		if (m_SceneState != SceneState::Edit)
			return;

		const auto fsPath = Project::AbsolueCopy(filePath);
		const AssetMetaData& metadata = ResourceManager::GetMetaData(fsPath);
		if (metadata.IsValid())
		{
			switch (metadata.Type)
			{
				case AssetType::Scene:
				{
					LoadScene(metadata.Handle);
					break;
				}
				case AssetType::Texture:
				{
					Ref<Texture2D> texture = ResourceManager::GetAsset<Texture2D>(metadata.Handle);
					if (texture)
					{
						Ref<AssetEditorPanel> assetEditor = m_PanelManager->GetPanel<AssetEditorPanel>(ASSET_EDITOR_ID);
						assetEditor->AddEditor<TextureEditorPanel>(texture->Handle, "Texture Editor", true, texture);
					}
					break;
				}
			}
		}
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

		UI_Info();
		UI_Shaders();
		UI_EditorCamera();
		UI_Settings();
		UI_CameraPrevie();
		UI_Stats();
		UI_ProjectSettings();
		UI_ImportTexture();
		UI_DebugScripts();

		m_PanelManager->OnImGuiRender();
		
		Theme::DrawThemeEditor(m_ShowThemeEditor);

		for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
			MethodThunks::OnUIRender(gcHandle);

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

				if (ImGui::MenuItem("Import Asset"))
					ImportAssetDialog();

				ImGui::Separator();

				if (ImGui::MenuItem("Open Project"))
					OpenProject();
				if (ImGui::BeginMenu("Recent Projects"))
				{
					// TODO(moro): Add Recent Projects
					ImGui::MenuItem("Sandbox");

					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Save Project"))
					SaveProject();

				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Reload Icons")) m_ReloadEditorIcons = true;
				ImGui::Separator();
				ImGui::MenuItem("Theme Editor", nullptr, &m_ShowThemeEditor);

				ImGui::EndMenu();
			}

			m_PanelManager->DrawPanelsMenu();
			if (ImGui::BeginMenu("Panels"))
			{
				ImGui::Separator();
				ImGui::MenuItem("Project", nullptr, &m_ShowProjectSettings);
				ImGui::MenuItem("Shaders", nullptr, &m_ShowShaders);
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
				ImGui::MenuItem("Info", nullptr, &m_ShowInfo);
				ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
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
		ImGui::BeginEx("MainViewport", m_MainViewportID);
		ImGui::PopStyleVar(4);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const bool anyItemFocused = ImGui::IsAnyItemActive() && GImGui->ActiveId != window->MoveId;
		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered || anyItemFocused);

		const ImVec2 size = ImGui::GetContentRegionAvail();
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportPos = ImGui::GetWindowPos();
			m_NeedsResize = true;
		}

		UI::SetBlend(false);
		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalImage();
		ImGui::Image(fbImage->GetViewID(), size);
		UI::SetBlend(true);

		UI_Gizmo();
		UI_DragDrop();

		ImGui::End();
	}

	void EditorLayer::UI_Gizmo()
	{
		SK_PROFILE_FUNCTION();
		
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

	void EditorLayer::UI_Info()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowInfo)
		{
			ImGui::Begin("Info", &m_ShowInfo);

			if (m_ViewportHovered && m_HoveredEntityID > -1)
			{
				Entity e{ (entt::entity)m_HoveredEntityID, m_ActiveScene };
				if (e.IsValid())
				{
					const auto& tag = e.GetComponent<TagComponent>().Tag;
					ImGui::Text("Hovered Entity: ID: %d, Tag: %s", m_HoveredEntityID, tag.c_str());
				}
				else
				{
					ImGui::Text("Hovered Entity: ID: %d", m_HoveredEntityID);
				}
			}
			else
			{
				ImGui::Text("Hovered Entity: InValid Entity, ID: %d", m_HoveredEntityID);
			}
			
			if (Entity entity{ (entt::entity)m_SelectetEntity, m_ActiveScene })
			{
				const auto& tag = entity.GetComponent<TagComponent>().Tag;
				ImGui::Text("Selected Entity: ID: %d, Tag: %s", (uint32_t)m_SelectetEntity, tag.c_str());
			}
			else
			{
				ImGui::Text("Selected Entity: InValid");
			}

			ImGui::End();
		}
	}

	void EditorLayer::UI_Shaders()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ShowShaders)
			return;

		if (ImGui::Begin("Shaders", &m_ShowShaders))
		{
			for (auto&& [key, shader] : *Renderer::GetShaderLib())
			{
				if (ImGui::TreeNodeEx(key.c_str()))
				{
					UI::Text(fmt::format("Path: {}", shader->GetFilePath()));
					if (ImGui::Button("ReCompile"))
						shader->ReCompile();
					if (ImGui::Button("Reflect"))
						shader->LogReflection();
					// IDEA: Print Shader Detailes (From Reflection)
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

	void EditorLayer::UI_EditorCamera()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowEditorCameraControlls)
		{
			if (ImGui::Begin("Editor Camera", &m_ShowEditorCameraControlls))
			{
				UI::BeginControls();

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

		if (ImGui::BeginDragDropTarget())
		{
			// Asset
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET");
				if (payload)
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;
					if (ResourceManager::IsValidAssetHandle(handle))
					{
						const AssetMetaData& metaData = ResourceManager::GetMetaData(handle);
						if (metaData.IsValid())
						{
							switch (metaData.Type)
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
									const auto& metadata = ResourceManager::GetMetaData(handle);
									m_TextureAssetCreateData.Clear();
									auto sourcePath = ResourceManager::GetFileSystemPath(metadata);
									m_TextureAssetCreateData.TextureSourcePath = sourcePath.string();
									m_TextureAssetCreateData.TextureFileName = sourcePath.stem().string();
									m_TextureAssetCreateData.OpenPopup = true;
									m_TextureAssetCreateData.CreateEntityAfterCreation = true;
									break;
								}
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
					std::filesystem::path filePath = std::wstring((const wchar_t*)payload->Data, payload->DataSize / sizeof(wchar_t));
					Project::Absolue(filePath);
					const auto extention = filePath.extension();
					
					if (extention == L".skscene")
					{
						AssetHandle handle = ResourceManager::ImportAsset(filePath);
						LoadScene(handle);
					}
					else if (extention == "L.sktex")
					{
						AssetHandle handle = ResourceManager::ImportAsset(filePath);
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

	}

	void EditorLayer::UI_ToolBar()
	{
		SK_PROFILE_FUNCTION();
		
		constexpr ImGuiWindowFlags falgs = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

		ImGuiStyle& style = ImGui::GetStyle();
		
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

		const float size = ImGui::GetContentRegionAvail().y;

		const auto imageButton = [this, size](auto strid, auto texture) { return ImGui::ImageButtonEx(UI::GetID(strid), texture->GetViewID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0, 0 }, { 0, 0, 0, 0 }, { 1, 1, 1, 1 }); };
		const auto imageButtonDisabled = [this, size](auto strid, auto texture) { ImGui::Image(texture->GetViewID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f, 0.5f, 1.0f }); };

		//imageButton("CursorIcon", m_CursorIcon);
		//ImGui::SameLine();
		//imageButton("TranslateIcon", m_TranslateIcon);
		//ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() * 0.5f - (size * 3.0f * 0.5f) - (style.ItemSpacing.x * 3.0f));

		//
		// Layout          [Play/Stop] [Simluate/Pause] [Step Disabled]
		// Layout Paused   [Stop]      [UnPause]        [Step]
		//
		// [UnPause] => either SceneState::Play || SceneState::Simulate
		//
		// 
		// m_ScenePause == true:
		// 
		//    SceneState::Editor:
		//       Error Not Possible
		//    
		//    SceneState::Play:
		//       [Stop] => [Stop]
		//       [UnPause] => [Unpause]
		//       [Step] => [Step]
		//    
		//    SceneState::Simulate:
		//       [Stop] => [Stop]
		//       [UnPause] => [Unpause]
		//       [Step] => [Step]
		// 
		// 
		// m_ScenePaused == false:
		// 
		//    SceneState::Editor:
		//       [Play/Stop] => [Play]
		//       [Simluate/Pause] => [Simulate]
		//       [Step] => [Step Disabled]
		//   
		//    SceneState::Play:
		//       [Play/Stop] => [Stop]
		//       [Simulate/Pause] => [Pause]
		//       [Step] => [Step Disabled]
		//   
		//    SceneState::Simulate:
		//       [Play/Stop] => [Stop]
		//       [Simulate/Pause] => [Pause]
		//       [Step] => [Step Disabled]
		//

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				// [Play]
				if (imageButton("PlayIcon", Icons::PlayIcon))
					OnScenePlay();

				// [Simulate]
				ImGui::SameLine();
				if (imageButton("SimulateIcon", Icons::SimulateIcon))
					OnSimulateStart();

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", Icons::StepIcon);

				break;
			}
			case SceneState::Play:
			{
				// [Stop]
				if (imageButton("StopIcon", Icons::StopIcon))
					OnSceneStop();

				// [Pause]
				ImGui::SameLine();
				if (imageButton("Pause", Icons::PauseIcon))
					m_SceneState = SceneState::Pause;

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", Icons::StepIcon);

				break;
			}
			case SceneState::Simulate:
			{
				// [Stop]
				if (imageButton("StopIcon", Icons::StopIcon))
					OnSceneStop();

				// [Pause]
				ImGui::SameLine();
				if (imageButton("Pause", Icons::PauseIcon))
					m_SceneState = SceneState::Pause;

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", Icons::StepIcon);

				break;
			}
			case SceneState::Pause:
			{
				// [Stop]
				if (imageButton("StopIcon", Icons::StopIcon))
					OnSceneStop();

				// [UnPause]
				ImGui::SameLine();
				if (imageButton("UnPause", Icons::PlayIcon))
					m_SceneState = m_InitialSceneState;

				// [Step]
				ImGui::SameLine();
				if (imageButton("Step", Icons::StepIcon))
					m_UpdateNextFrame = true;

				break;
			}
		}

		ImGui::End();
	}

	void EditorLayer::UI_Settings()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ShowSettings)
		{
			if (ImGui::Begin("Settings", &m_ShowSettings))
			{
				m_SceneRenderer->OnImGuiRender();

				if (ImGui::CollapsingHeader("Visualization"))
				{
					if (ImGui::TreeNodeEx("View", UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::BeginControlsGrid();
						UI::Control("Camera Preview", m_ShowCameraPreview);
						UI::EndControls();
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Colliders", UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::BeginControlsGrid();
						UI::Control("Show", m_ShowColliders);
						UI::Control("OnTop", m_ShowCollidersOnTop);
						UI::EndControls();
						ImGui::TreePop();
					}
				}

				if (ImGui::CollapsingHeader("Stuff"))
				{
					UI::BeginControlsGrid();
					auto& window = Application::Get().GetWindow();
					bool vSync = window.IsVSync();
					if (UI::Control("VSync", vSync))
						window.SetVSync(vSync);

					UI::Control("Read Hoved Entity", m_ReadHoveredEntity);
					UI::EndControls();
				}

				if (ImGui::CollapsingHeader("Profile"))
				{
					if (ImGui::TreeNodeEx("Box2D", UI::TreeNodeSeperatorFlags))
					{
						auto& physicsScene = m_ActiveScene->GetPhysicsScene();
						if (physicsScene.Active())
						{
							auto& profile = physicsScene.GetProfile();

							ImGui::Text("TimeStep: %f", profile.TimeStep);
							ImGui::Text("Steps: %d", profile.NumSteps);

							std::map<float, std::string, std::greater<>> times;
							times[profile.Step] = "Step";
							times[profile.Collide] = "Collide";
							times[profile.Solve] = "Solve";
							times[profile.SolveInit] = "SolveInit";
							times[profile.SolveVelocity] = "SolveVelocity";
							times[profile.SolvePosition] = "SolvePosition";
							times[profile.Broadphase] = "Broadphase";
							times[profile.SolveTOI] = "SolveTOI";

							UI::BeginControlsGrid();
							for (auto& [time, name] : times)
								UI::Property(name, fmt::to_string(time));
							UI::EndControls();

							//UI::Control("Step",           fmt::to_string(profile.step));
							//UI::Control("Collide",        fmt::to_string(profile.collide));
							//UI::Control("Solve",          fmt::to_string(profile.solve));
							//UI::Control("Solve Init",     fmt::to_string(profile.solveInit));
							//UI::Control("Solve Velocity", fmt::to_string(profile.solveVelocity));
							//UI::Control("Solve Position", fmt::to_string(profile.solvePosition));
							//UI::Control("BroadPhase",     fmt::to_string(profile.broadphase));
							//UI::Control("Solve TOI",      fmt::to_string(profile.solveTOI));
						}
						else
						{
							UI::Text("Phyics Scene inactive");
						}
						ImGui::TreePop();
					}

				}

			}
			ImGui::End();
		}
	}

	static void CameraPreviewResizeCallback(ImGuiSizeCallbackData* data)
	{
		SK_PROFILE_FUNCTION();

		ImVec2 viewportSize = *(ImVec2*)data->UserData;
		float aspectRatio = viewportSize.x / viewportSize.y;
		ImVec2 delta = data->DesiredSize - data->CurrentSize;

		if (delta.x != 0.0f && delta.y != 0.0f)
		{
			if (delta.x > delta.y)
			{
				data->DesiredSize.y = data->DesiredSize.x / aspectRatio;
			}
			else
			{
				data->DesiredSize.x = data->DesiredSize.y * aspectRatio;
			}
		}
		else if (delta.x != 0.0f)
		{
			data->DesiredSize.y = data->DesiredSize.x / aspectRatio;
		}
		else if (delta.y != 0.0f)
		{
			data->DesiredSize.x = data->DesiredSize.y * aspectRatio;
		}
	}

	void EditorLayer::UI_CameraPrevie()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ShowCameraPreview && m_SceneState != SceneState::Play)
		{
			ImVec2 viewportSize = { (float)m_ViewportWidth, (float)m_ViewportHeight };
			ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX }, CameraPreviewResizeCallback, &viewportSize);

			ImGui::Begin("Camera Preview", nullptr, ImGuiWindowFlags_NoTitleBar);

			UI::SetBlend(false);
			Ref<Image2D> image = m_CameraPreviewRenderer->GetFinalImage();
			ImGui::Image(image->GetViewID(), ImGui::GetContentRegionAvail());
			UI::SetBlend(true);

			ImGui::End();
		}
	}

	void EditorLayer::UI_Stats()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_ShowStats)
			return;

		ImGui::Begin("Debug Renderer");
		const Renderer2D::Statistics& s = m_DebugRenderer->GetStatistics();
		ImGui::Text("Draw Calls: %d", s.DrawCalls);
		ImGui::Text("Quad Count: %d", s.QuadCount);
		ImGui::Text("Circle Count: %d", s.CircleCount);
		ImGui::Text("Line Count: %d", s.LineCount);
		ImGui::Text("Line On Top Count: %d", s.LineOnTopCount);
		ImGui::Text("Vertex Count: %d", s.VertexCount);
		ImGui::Text("Index Count: %d", s.IndexCount);
		ImGui::Text("Texture Count: %d", s.TextureCount);
		ImGui::End();


		ImGui::Begin("Times");
		UI::Text(fmt::format("Mouse Picking: {:.4f}ms", ProfilerRegistry::GetAverageOf("Mouse Picking").MilliSeconds()));
		UI::Text(fmt::format("Window Update: {:.4f}ms", ProfilerRegistry::GetAverageOf("WindowsWindow::Update").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Editor: {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderEditor").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Runtime: {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderRuntime").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Simulate: {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderSimulate").MilliSeconds()));
		UI::Text(fmt::format("Sceme Render Runtime Preview: {:.4f}ms", ProfilerRegistry::GetAverageOf("Scene::OnRenderRuntimePreview").MilliSeconds()));
		ImGui::End();


		ImGui::Begin("GPU Times");
		//UI::Text(fmt::format("Present GPU: {:.4f}ms", Renderer::GetPresentTimer()->GetTime().MilliSeconds()));
		UI::Text(fmt::format("ImGuiLayer GPU: {:.4f}ms", ProfilerRegistry::GetAverageOf("[GPU] DirectXImGuiLayer::End").MilliSeconds()));
		ImGui::End();


		ImGui::Begin("CPU Times");
		UI::Text(fmt::format("FrameTime: {:.4f}ms", m_TimeStep.MilliSeconds()));
		UI::Text(fmt::format("Present CPU: {:.4f}ms", ProfilerRegistry::GetAverageOf("SwapChain::Present").MilliSeconds()));
		UI::Text(fmt::format("NewFrame: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::NewFrame").MilliSeconds()));
		UI::Text(fmt::format("Renderer2D EndScene: {:.4f}ms", ProfilerRegistry::GetAverageOf("Renderer2D::EndScene").MilliSeconds()));
		UI::Text(fmt::format("RenderGeometry: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry").MilliSeconds()));
		UI::Text(fmt::format("RenderFullScreenQuad: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuad").MilliSeconds()));
		ImGui::End();

	}

	void EditorLayer::UI_ProjectSettings()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ShowProjectSettings)
			return;

		ImGui::Begin("Project", &m_ShowProjectSettings);

		auto& config = *Project::GetActive();
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

		UI::BeginControlsGrid();

		if (UI::ControlCustomBegin("Name"))
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputText("##Name", &config.Name);
			UI::ControlCustomEnd();
		}

		if (UI::ControlCustomBegin("Assets"))
		{
			UI::LagacyScopedStyleStack style;
			if (!m_ProjectEditData.ValidAssetsPath)
				style.Push(ImGuiCol_Text, Theme::Colors::TextInvalidInput);

			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##Assets", &m_ProjectEditData.Assets))
			{
				auto assetsDirectory = Project::AbsolueCopy(m_ProjectEditData.Assets);
				m_ProjectEditData.ValidAssetsPath = (std::filesystem::exists(assetsDirectory) && std::filesystem::is_directory(assetsDirectory));
				if (m_ProjectEditData.ValidAssetsPath)
					config.AssetsDirectory = assetsDirectory;
			}

			UI::ControlCustomEnd();
		}

		if (UI::ControlCustomBegin("StartupScene"))
		{
			UI::LagacyScopedStyleStack style;
			if (!m_ProjectEditData.ValidStartupScene)
				style.Push(ImGuiCol_Text, Theme::Colors::TextInvalidInput);

			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##StartupScene", &m_ProjectEditData.StartupScene))
			{
				auto startupScene = Project::AbsolueCopy(m_ProjectEditData.StartupScene);
				m_ProjectEditData.ValidStartupScene = std::filesystem::exists(startupScene) && std::filesystem::is_regular_file(startupScene) && (startupScene.extension() == L".skscene");
				if (m_ProjectEditData.ValidStartupScene)
					config.StartupScenePath = startupScene;
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET");
				if (payload)
				{
					SK_CORE_ASSERT(payload->DataSize == sizeof(AssetHandle), "Invalid Payload size");
					AssetHandle handle = *(AssetHandle*)payload->Data;
					const AssetMetaData& metadata = ResourceManager::GetMetaData(handle);
					if (metadata.Type == AssetType::Scene)
					{
						config.StartupScenePath = ResourceManager::GetFileSystemPath(metadata);
						m_ProjectEditData.StartupScene = Project::RelativeCopy(config.StartupScenePath).string();
						m_ProjectEditData.ValidStartupScene = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			UI::PopID();
		}

		UI::Control("Gravity", config.Gravity);
		UI::Control("Velocity Iterations", config.VelocityIterations);
		UI::Control("Position Iterations", config.PositionIterations);
		float fixedTSInMS = config.FixedTimeStep * 1000.0f;
		if (UI::Control("Fixed Time Step", fixedTSInMS, 0.1f, 0.1f, FLT_MAX, "%.3fms"))
			config.FixedTimeStep = fixedTSInMS * 0.001f;

		UI::EndControls();

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
			UI::Text("Input FileName");
			UI::Text(fmt::format("Parent Path: {}/Texture", Project::GetAssetsPath()));
			ImGui::InputText("##FileName", &m_TextureAssetCreateData.TextureFileName);

			if (ImGui::Button("Import"))
			{
				std::string directory = String::ToNarrowCopy(fmt::format(L"{}/Textures", Project::GetAssetsPath().native()));
				std::string fileName = std::filesystem::path(m_TextureAssetCreateData.TextureFileName).replace_extension(".sktex").string();

				Ref<Texture2D> texture = ResourceManager::CreateAsset<Texture2D>(directory, fileName, m_TextureAssetCreateData.TextureSourcePath);

				if (m_TextureAssetCreateData.CreateEntityAfterCreation)
				{
					Entity entity = m_ActiveScene->CreateEntity();
					auto& sr = entity.AddComponent<SpriteRendererComponent>();
					sr.TextureHandle = texture->Handle;
					SelectEntity(entity);
				}

				ImGui::CloseCurrentPopup();
			}

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

		SK_PERF_SCOPED("Mouse Picking");
		

		auto [mx, my] = ImGui::GetMousePos();
		auto [wx, wy] = m_ViewportPos;
		int x = (int)(mx - wx);
		int y = (int)(my - wy);
		m_HoveredEntityID = -1;

		const auto& specs = m_MousePickingImage->GetSpecification();
		int width = (int)specs.Width;
		int height = (int)specs.Height;
		if (x >= 0 && x < (int)width && y >= 0 && y < (int)height)
		{
			const bool selectEntity = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::IsKeyDown(KeyCode::LeftAlt) && m_ViewportHovered;

			if (m_ReadHoveredEntity || selectEntity)
			{
				Ref<Image2D> idImage = m_SceneRenderer->GetIDImage();
				if (!idImage->CopyTo(m_MousePickingImage))
					return false;

				if (!m_MousePickingImage->ReadPixel(x, y, (uint32_t&)m_HoveredEntityID))
					return false;
			}

			if (selectEntity)
			{
				if (m_HoveredEntityID != -1)
				{
					Entity entity{ (entt::entity)(uint32_t)m_HoveredEntityID, m_ActiveScene };
					SK_CORE_ASSERT(entity.IsValid());
					if (entity.IsValid())
						SelectEntity(entity);
				}
				else
				{
					SelectEntity({});
				}
			}
		}
		return true;
	}

	void EditorLayer::UI_DebugScripts()
	{
		SK_PROFILE_FUNCTION();

		ImGui::Begin("Debug Scripts");

		ImGui::Text("Scripts: %llu", ScriptEngine::GetEntityInstances().size());

		for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
		{
			const char* className = ScriptUtils::GetClassName(gcHandle);
			ImGui::TreeNodeEx(className, UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		}

		ImGui::End();
	}

	void EditorLayer::DebugRender()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowColliders)
		{
			//m_DebugRenderer->SetRenderTarget(m_SceneRenderer->GetExternalCompositFrameBuffer());
			m_DebugRenderer->BeginScene(GetActiveViewProjection());
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
			m_DebugRenderer->EndScene();
		}
	}

	void EditorLayer::RenderCameraPreview()
	{
		SK_PROFILE_FUNCTION();

		if (m_ShowCameraPreview)
		{
			Entity cameraEntity = m_ActiveScene->GetActiveCameraEntity();
			if (cameraEntity)
				m_ActiveScene->OnRenderRuntime(m_CameraPreviewRenderer);
		}
	}

	Entity EditorLayer::CreateEntity(const std::string& name)
	{
		SK_PROFILE_FUNCTION();

		return m_ActiveScene->CreateEntity(name);
	}

	void EditorLayer::DeleteEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		if (entity.GetUUID() == m_ActiveScene->GetActiveCameraUUID())
			m_ActiveScene->SetActiveCamera(UUID());

		m_ActiveScene->DestroyEntity(entity);
		SelectEntity(Entity{});
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		m_SelectetEntity = entity;
		m_PanelManager->GetPanel<SceneHirachyPanel>(SCENE_HIRACHY_ID)->SetSelectedEntity(entity);
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

	void EditorLayer::NewScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		Ref<Scene> newScene = ResourceManager::CreateMemoryAsset<Scene>();
		m_WorkScene = newScene;
		SetActiveScene(newScene);
		m_EditorCamera.SetView(glm::vec3(0.0f), 10.0f, 0.0f, 0.0f);
	}

	bool EditorLayer::LoadScene(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		auto& metadata = ResourceManager::GetMetaData(filePath);
		if (!metadata.IsValid())
			return false;

		Ref<Scene> scene = ResourceManager::GetAsset<Scene>(metadata.Handle);
		if (scene)
		{
			m_WorkScene = scene;
			SetActiveScene(scene);
			m_EditorCamera.SetView(glm::vec3(0.0f), 10.0f, 0.0f, 0.0f);
			return true;
		}
		return false;
	}

	bool EditorLayer::LoadScene(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		Ref<Scene> scene = ResourceManager::GetAsset<Scene>(handle);
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

		if (ResourceManager::IsMemoryAsset(m_WorkScene->Handle))
			return SaveSceneAs();

		return ResourceManager::SaveAsset(m_WorkScene->Handle);
	}

	bool EditorLayer::SaveSceneAs()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_WorkScene);

		auto filePath = PlatformUtils::SaveFileDialog(L"|*.*|Scene|*.skscene", 2, Project::GetAssetsPath(), true);
		if (filePath.empty())
			return false;

		SK_CORE_ASSERT(filePath.extension() == L".skscene");
		if (filePath.extension() != L".skscene")
			return false;

		if (ResourceManager::IsMemoryAsset(m_WorkScene->Handle))
		{
			std::string directoryPath = ResourceManager::MakeRelativePathString(filePath.parent_path());
			std::string fileName = filePath.filename().string();
			return ResourceManager::ImportMemoryAsset(m_WorkScene->Handle, directoryPath, fileName);
		}

		std::string directoryPath = ResourceManager::MakeRelativePathString(filePath.parent_path());
		std::string fileName = filePath.filename().string();
		Ref<Scene> newScene = ResourceManager::CreateAsset<Scene>(directoryPath, fileName);
		m_WorkScene->CopyTo(newScene);
		return ResourceManager::SaveAsset(newScene->Handle);
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		m_InitialSceneState = SceneState::Play;

		m_CurrentOperation = GizmoOperaton::None;

		SetActiveScene(Scene::Copy(m_WorkScene));
		ScriptEngine::InitializeRuntime(m_ActiveScene);
		Application::Get().QueueEvent<ScenePlayEvent>(m_ActiveScene);
		m_ActiveScene->OnScenePlay();
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_InitialSceneState = SceneState::None;

		m_ActiveScene->OnSceneStop();
		ScriptEngine::ShutdownRuntime();
		SetActiveScene(m_WorkScene);
	}

	void EditorLayer::OnSimulateStart()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Simulate;
		m_InitialSceneState = SceneState::Simulate;

		SetActiveScene(Scene::Copy(m_WorkScene));
		m_ActiveScene->OnSimulationPlay();
	}

	void EditorLayer::SetActiveScene(Ref<Scene> scene)
	{
		SK_PROFILE_FUNCTION();

		if (m_ActiveScene && ResourceManager::IsMemoryAsset(m_ActiveScene->Handle))
			ResourceManager::UnloadAsset(m_ActiveScene->Handle);

		if (scene && m_SelectetEntity)
			SelectEntity(scene->GetEntityByUUID(m_SelectetEntity.GetUUID()));
		else
			SelectEntity(Entity{});

		if (scene)
		{
			scene->IsEditorScene(m_InitialSceneState != SceneState::Play);
			scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}

		m_ActiveScene = scene;
		m_SceneRenderer->SetScene(scene);
		m_CameraPreviewRenderer->SetScene(scene);
		Application::Get().QueueEvent<SceneChangedEvent>(scene);
	}

	void EditorLayer::OpenProject()
	{
		SK_PROFILE_FUNCTION();

		auto filePath = PlatformUtils::OpenFileDialog(L"|*.*|Project|*.skproj", 2);
		if (!filePath.empty())
			OpenProject(filePath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("Opening Project [{}]", filePath);

		auto project = Ref<ProjectInstance>::Create();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(filePath))
		{
			if (Project::GetActive())
				CloseProject();

			Project::SetActive(project);
			ResourceManager::Init();

			ScriptEngine::LoadAssemblies(Project::GetActive()->ScriptModulePath);

			if (!LoadScene(project->Directory / project->StartupScenePath))
				NewScene();

			Application::Get().QueueEvent<ProjectChangedEvent>(project);
			FileSystem::StartWatching(Project::GetAssetsPath());

			m_ProjectEditData = project;
		}
	}

	void EditorLayer::CloseProject()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(Project::GetActive());

		SaveProject();

		SK_CORE_INFO("Closing Project");

		ResourceManager::Shutdown();

		m_ActiveScene = nullptr;
		m_SceneRenderer->SetScene(nullptr);
		m_CameraPreviewRenderer->SetScene(nullptr);
		m_PanelManager->GetPanel<SceneHirachyPanel>(SCENE_HIRACHY_ID)->SetContext(nullptr);
		m_PanelManager->GetPanel<PhysicsDebugPanel>(PHYSICS_DEBUG_ID)->SetContext(nullptr);

		SetActiveScene(nullptr);

		// Note(moro): When CloseProject gets called durring application shutdown the application can't Raise Events anymore
		//             to get around this the event is distributed internal
		if (!Application::Get().CanRaiseEvents())
			OnEvent(SceneChangedEvent(nullptr));
		ScriptEngine::UnloadAssemblies();

		SK_CORE_ASSERT(m_WorkScene->GetRefCount() == 1);
		m_WorkScene = nullptr;

		FileSystem::StopWatching();

		Project::SetActive(nullptr);
		Application::Get().QueueEvent<ProjectChangedEvent>(nullptr);
	}

	void EditorLayer::SaveProject()
	{
		SK_CORE_ASSERT(Project::GetActive());
		SaveProject(Project::GetDirectory() / "Project.skproj");
	}

	void EditorLayer::SaveProject(const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(Project::GetActive());
		ProjectSerializer serializer(Project::GetActive());
		serializer.Serialize(filePath);
	}

	void EditorLayer::ImportAssetDialog()
	{
		auto results = PlatformUtils::OpenFileDialogMuliSelect(L"", 1, Project::GetAssetsPath());
		for (const auto& path : results)
		{
			if (!ResourceManager::IsFileImported(path))
				ResourceManager::ImportAsset(path);
		}
	}

	void EditorLayer::RunScriptSetup()
	{
		ExecuteSpecs specs;
		specs.Target = fmt::format(L"{}/Premake/premake5.exe", Project::GetDirectory());
		// vs2022 dosn't work for some reason but vs2019 still generates vs2022 solution
		auto sharkDir = std::filesystem::current_path().parent_path();
		specs.Params = L"vs2019";
		specs.WaitUntilFinished = true;
		specs.WorkingDirectory = Project::GetDirectory();
		specs.InterhitConsole = true;
		PlatformUtils::Execute(specs);
	}

	void EditorLayer::OpenIDE()
	{
		auto solutionPath = fmt::format("{}/{}.sln", Project::GetDirectory(), Project::GetName());
		PlatformUtils::Execute(ExectueVerb::Open, solutionPath);
	}

}
