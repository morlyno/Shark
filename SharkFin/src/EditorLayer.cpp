#include "skfpch.h"
#include "EditorLayer.h"

#include "Shark/Core/Project.h"
#include <Shark/Scene/Components.h>
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Asset/SceneSerialization.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/UI/UI.h"

#include "Shark/File/FileSystem.h"

#include "Shark/Utils/PlatformUtils.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"
#include "Panels/PhysicsDebugPanel.h"
#include "Shark/Editor/EditorConsole/EditorConsolePanel.h"
#include "Shark/Editor/Icons.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/enttDebug.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#define SCENE_HIRACHY_ID "SceneHirachyPanel"
#define CONTENT_BROWSER_ID "ContentBrowserPanel"
#define TEXTURE_EDITOR_ID "TextureEditorPanel"
#define PHYSICS_DEBUG_ID "PhysicsDebugPanel"
#define ASSET_EDITOR_ID "AssetsEditorPanel"
#define EDITOR_CONSOLE_ID "EditorConsolePanel"

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

		// Create and setup Panels
		m_PanelManager = Scope<PanelManager>::Create();

		auto sceneHirachy = m_PanelManager->AddPanel<SceneHirachyPanel>(SCENE_HIRACHY_ID, true);
		sceneHirachy->SetSelectionChangedCallback([this](Entity entity) { m_SelectetEntity = entity; });

		m_PanelManager->AddPanel<PhysicsDebugPanel>(PHYSICS_DEBUG_ID, true);
		m_PanelManager->AddPanel<AssetEditorPanel>(ASSET_EDITOR_ID, false);
		m_PanelManager->AddPanel<EditorConsolePanel>(EDITOR_CONSOLE_ID, true);

		auto contentBrowserPanel = m_PanelManager->AddPanel<ContentBrowserPanel>(CONTENT_BROWSER_ID, true);
		contentBrowserPanel->SetOpenFileCallback([this](auto& fs) { this->OnFileClickedCallback(fs); });

		// Renderer stuff
		auto& window = Application::Get().GetWindow();
		m_ViewportWidth = window.GetWidth();
		m_ViewportHeight = window.GetHeight();

		m_EditorCamera.SetProjection(16.0f / 9.0f, 45, 0.01f, 1000.0f);

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

		FileSystem::SetFileWatcherCallback(std::bind(&EditorLayer::OnFileChanged, this, std::placeholders::_1));
	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		CloseProject();
		FileSystem::SetFileWatcherCallback(nullptr);

		m_PanelManager->Clear();

		Icons::Shutdown();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		Renderer::NewFrame();

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

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

		const bool control = Input::KeyPressed(Key::Control);
		const bool shift = Input::KeyPressed(Key::LeftShift);

		switch (event.GetKeyCode())
		{

			// New Scene
			case Key::N:
			{
				if (control)
				{
					NewScene();
					return true;
				}
				break;
			}

			// Save Scene
			case Key::S:
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
			case Key::D:
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
			case Key::F:
			{
				if (m_SelectetEntity)
				{
					const auto& tf = m_SelectetEntity.Transform();
					m_EditorCamera.SetFocusPoint(tf.Translation);
					m_EditorCamera.SetDistance(7.5f);
					return true;
				}
				break;
			}

			case Key::Delete:
			{
				if (m_SelectetEntity)
					DeleteEntity(m_SelectetEntity);
				return true;
			}

			// Toggle VSync
			case Key::V:
			{
				auto& window = Application::Get().GetWindow();
				window.SetVSync(!window.IsVSync());
				return true;
			}

			// ImGuizmo
			case Key::Q: { m_CurrentOperation = GizmoOperaton::None; return true; }
			case Key::W: { m_CurrentOperation = GizmoOperaton::Translate; return true; }
			case Key::E: { m_CurrentOperation = GizmoOperaton::Rotate; return true; }
			case Key::R: { m_CurrentOperation = GizmoOperaton::Scale; return true; }
		}

		return false;
	}

	void EditorLayer::OnFileChanged(const std::vector<FileChangedData>& fileEvents)
	{
		SK_PROFILE_FUNCTION();

		ResourceManager::OnFileEvents(fileEvents);
		m_PanelManager->GetPanel<ContentBrowserPanel>(CONTENT_BROWSER_ID)->OnFileChanged(fileEvents);
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
						assetEditor->AddEditor<TextureEditorPanel>(texture->Handle, true, texture);
					}
					break;
				}
			}
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

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
		UI_Asset();
		UI_ImportTexture();
		UI_DebugScripts();

		m_PanelManager->OnImGuiRender();

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

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Scene Hirachy", nullptr, m_PanelManager->IsShown(SCENE_HIRACHY_ID))) { m_PanelManager->ToggleShow(SCENE_HIRACHY_ID); }
				if (ImGui::MenuItem("Content Browser", nullptr, m_PanelManager->IsShown(CONTENT_BROWSER_ID))) { m_PanelManager->ToggleShow(CONTENT_BROWSER_ID); }
				if (ImGui::MenuItem("Console", nullptr, m_PanelManager->IsShown(EDITOR_CONSOLE_ID))) { m_PanelManager->ToggleShow(EDITOR_CONSOLE_ID); }
				if (ImGui::MenuItem("Physics Debug", nullptr, m_PanelManager->IsShown(PHYSICS_DEBUG_ID))) { m_PanelManager->ToggleShow(PHYSICS_DEBUG_ID); }
				ImGui::Separator();
				ImGui::MenuItem("Project", nullptr, &m_ShowProjectSettings);
				ImGui::MenuItem("Shaders", nullptr, &m_ShowShaders);
				ImGui::MenuItem("Assets", nullptr, &m_ShowAssets);
				ImGui::Separator();
				ImGui::MenuItem("Physics Debug", nullptr, m_PanelManager->IsShown(PHYSICS_DEBUG_ID));
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script", m_SceneState == SceneState::Edit))
			{
				if (ImGui::MenuItem("Reload", nullptr, nullptr, m_SceneState == SceneState::Edit))
				{
					ScriptEngine::ReloadAssemblies(Project::ScriptModulePath());
					UpdateScriptComponents();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Auto Reload", nullptr, m_AssemblyReloadMode == AssemblyReloadMode::Auto))
					m_AssemblyReloadMode = m_AssemblyReloadMode == AssemblyReloadMode::Auto ? AssemblyReloadMode::None : AssemblyReloadMode::Auto;

				if (ImGui::MenuItem("Hot Reload", nullptr, m_AssemblyReloadMode == AssemblyReloadMode::Always))
					m_AssemblyReloadMode = m_AssemblyReloadMode == AssemblyReloadMode::Always ? AssemblyReloadMode::None : AssemblyReloadMode::Always;

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("MainViewport");
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		const ImVec2 size = ImGui::GetContentRegionAvail();

		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportPos = ImGui::GetWindowPos();
			m_NeedsResize = true;
		}

		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalImage();

		UI::SetBlend(false);
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

			glm::mat4 transform = m_ActiveScene->GetWorldSpaceTransform(m_SelectetEntity);

			float snapVal = 0.0f;
			if (Input::KeyPressed(Key::LeftShift))
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

			if (!Input::KeyPressed(Key::Alt) && ImGuizmo::IsUsing() || Input::KeyPressed(Key::Space))
			{
				Entity parentEntity = m_SelectetEntity.Parent();
				glm::mat4 localTransform = parentEntity ? glm::inverse(m_ActiveScene->GetWorldSpaceTransform(parentEntity)) * transform : transform;

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 2 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, style.ItemSpacing.y });
		auto colHovered = style.Colors[ImGuiCol_ButtonHovered];
		auto colActive = style.Colors[ImGuiCol_ButtonActive];
		colHovered.w = colActive.w = 0.5f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
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

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
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

							std::map<float, std::string, std::greater<float>> times;
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
								UI::Control(name, fmt::to_string(time));
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

		auto& config = Project::GetActive()->GetConfig();
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
			UI::ScopedStyle style;
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
			UI::ScopedStyle style;
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

		UI::Control("Gravity", config.Gravity, { 0.0f, -9.81 });
		UI::Control("Velocity Iterations", config.VelocityIterations, 8);
		UI::Control("Position Iterations", config.PositionIterations, 3);
		float fixedTSInMS = config.FixedTimeStep * 1000.0f;
		if (UI::Control("Fixed Time Step", fixedTSInMS, 1.0f, 0.1f, FLT_MAX, 1.0f, "%.3fms"))
			config.FixedTimeStep = fixedTSInMS * 0.001f;

		UI::EndControls();

		ImGui::End();
	}

	void EditorLayer::UI_Asset()
	{
		SK_PROFILE_FUNCTION();

		if (!m_ShowAssets)
			return;

		ImGui::Begin("Assets", &m_ShowAssets);

		static std::string SearchBuffer;
		ImGui::SetNextItemWidth(-1.0f);
		static bool SearchHasUppercase = false;
		if (ImGui::InputText("##SearchBuffer", &SearchBuffer))
		{
			SearchHasUppercase = false;
			for (auto& c : SearchBuffer)
			{
				if (isupper(c))
				{
					SearchHasUppercase = true;
					break;
				}
			}
		}

		if (ImGui::TreeNodeEx("Imported Assets", UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (auto [handle, metadata] : ResourceManager::GetAssetRegistry())
			{
				if (!SearchBuffer.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					std::string typeStr = AssetTypeToString(metadata.Type);
					std::string filePathStr = metadata.FilePath.string();

					if (!SearchHasUppercase)
					{
						String::ToLower(handleStrHex);
						String::ToLower(typeStr);
						String::ToLower(filePathStr);
					}

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= filePathStr.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}


				UI::BeginControlsGrid();

				const UI::TextFlags textFlags = UI::TextFlag::Selectable | UI::TextFlag::Aligned;
				UI::Control("Handle", fmt::format("{:x}", metadata.Handle), textFlags);
				UI::Control("FilePath", metadata.FilePath, textFlags);
				UI::Control("Type", AssetTypeToString(metadata.Type), textFlags);

				UI::EndControls();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Loaded Assets", UI::TreeNodeSeperatorFlags))
		{
			for (auto [handle, asset] : ResourceManager::GetLoadedAssets())
			{
				const auto& metadata = ResourceManager::GetMetaData(asset);

				if (!SearchBuffer.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					std::string typeStr = AssetTypeToString(metadata.Type);
					std::string filePathStr = metadata.FilePath.string();

					if (!SearchHasUppercase)
					{
						String::ToLower(handleStrHex);
						String::ToLower(typeStr);
						String::ToLower(filePathStr);
					}

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= filePathStr.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}


				UI::BeginControlsGrid();

				const UI::TextFlags textFlags = UI::TextFlag::Selectable | UI::TextFlag::Aligned;
				UI::Control("Handle", fmt::format("{:x}", metadata.Handle), textFlags);
				UI::Control("FilePath", metadata.FilePath, textFlags);
				UI::Control("Type", AssetTypeToString(metadata.Type), textFlags);

				UI::EndControls();
			}

			ImGui::TreePop();
		}
		
		if (ImGui::TreeNodeEx("Memory Assets", UI::TreeNodeSeperatorFlags))
		{
			for (auto [handle, asset] : ResourceManager::GetMemoryAssets())
			{
				if (!SearchBuffer.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", handle);
					std::string typeStr = AssetTypeToString(asset->GetAssetType());

					if (!SearchHasUppercase)
					{
						String::ToLower(handleStrHex);
						String::ToLower(typeStr);
					}

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}


				UI::BeginControlsGrid();

				const UI::TextFlags textFlags = UI::TextFlag::Selectable | UI::TextFlag::Aligned;
				UI::Control("Handle", fmt::format("{:x}", handle), textFlags);
				UI::Control("Type", AssetTypeToString(asset->GetAssetType()), textFlags);

				UI::EndControls();
			}

			ImGui::TreePop();
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
			UI::Text("Input FileName");
			UI::Text(fmt::format("Parent Path: {}/Texture", Project::AssetsPath()));
			ImGui::InputText("##FileName", &m_TextureAssetCreateData.TextureFileName);

			if (ImGui::Button("Import"))
			{
				std::string directory = String::ToNarrowCopy(fmt::format(L"{}/Textures", Project::AssetsPath().native()));
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
			const bool selectEntity = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !Input::KeyPressed(Key::Alt) && m_ViewportHovered;

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

			if (m_ShowCollidersOnTop)
			{
				{
					auto view = m_ActiveScene->GetAllEntitysWith<BoxCollider2DComponent>();
					for (auto entityID : view)
					{
						Entity entity{ entityID, m_ActiveScene };
						auto& collider = view.get<BoxCollider2DComponent>(entityID);
						auto& tf = entity.Transform();

						glm::mat4 transform =
							tf.CalcTransform() *
							glm::translate(glm::vec3(collider.Offset, 0)) *
							glm::rotate(collider.Rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
							glm::scale(glm::vec3(collider.Size * 2.0f, 1.0f));
						
						m_DebugRenderer->DrawRectOnTop(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
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
							tf.CalcTransform() *
							glm::translate(glm::vec3(collider.Offset, 0)) *
							glm::rotate(collider.Rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
							glm::scale(glm::vec3(collider.Radius, collider.Radius, 1.0f));

						m_DebugRenderer->DrawCircleOnTop(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
					}
				}
			}
			else
			{
				{
					auto view = m_ActiveScene->GetAllEntitysWith<BoxCollider2DComponent>();
					for (auto entityID : view)
					{
						Entity entity{ entityID, m_ActiveScene };
						auto& collider = view.get<BoxCollider2DComponent>(entityID);
						auto& tf = entity.Transform();

						glm::mat4 transform =
							tf.CalcTransform() *
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
							tf.CalcTransform() *
							glm::translate(glm::vec3(collider.Offset, 0)) *
							glm::rotate(collider.Rotation, glm::vec3(0.0f, 0.0f, 1.0f)) *
							glm::scale(glm::vec3(0.0f, 0.0f, collider.Radius));

						m_DebugRenderer->DrawCircle(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
					}
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
			return camera.GetProjection() * glm::inverse(m_ActiveScene->GetWorldSpaceTransform(cameraEntity));
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

		if (ResourceManager::IsMemoryAsset(m_WorkScene->Handle))
		{
			auto filePath = PlatformUtils::SaveFileDialog(L"|*.*|Scene|*.skscene", 2, Project::AssetsPath(), true);
			if (!filePath.empty())
			{
				SK_CORE_ASSERT(filePath.extension() == L".skscene");

				std::string directoryPath = ResourceManager::MakeRelativePathString(filePath.parent_path());
				std::string fileName = filePath.filename().string();
				return ResourceManager::ImportMemoryAsset(m_ActiveScene->Handle, directoryPath, fileName);
			}
			return false;
		}

		SK_CORE_ASSERT(false, "Normal assets currently cant be saved at a different location");
		return false;
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		m_InitialSceneState = SceneState::Play;

		m_CurrentOperation = GizmoOperaton::None;

		SetActiveScene(Scene::Copy(m_WorkScene));

		if (m_AssemblyReloadMode == AssemblyReloadMode::Always)
		{
			ScriptEngine::ReloadAssemblies(Project::ScriptModulePath());
			UpdateScriptComponents();
		}
		else if (m_AssemblyReloadMode == AssemblyReloadMode::Auto)
		{
			SK_CORE_ERROR("AssemblyReloadMode::Auto currently not supported");
			m_AssemblyReloadMode = AssemblyReloadMode::Always;
			ScriptEngine::ReloadAssemblies(Project::ScriptModulePath());
			//ScriptEngine::ReloadIfNeeded();
			UpdateScriptComponents();
		}

		ScriptEngine::SetActiveScene(m_ActiveScene);
		DistributeEvent(ScenePlayEvent(m_ActiveScene));
		m_ActiveScene->OnScenePlay();
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_InitialSceneState = SceneState::None;

		m_ActiveScene->OnSceneStop();
		ScriptEngine::SetActiveScene(nullptr);
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

		SelectEntity(Entity{});

		if (scene)
		{
			scene->IsEditorScene(m_InitialSceneState != SceneState::Play);
			scene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}

		m_ActiveScene = scene;
		m_SceneRenderer->SetScene(scene);
		m_CameraPreviewRenderer->SetScene(scene);
		SelectEntity(Entity{});
		DistributeEvent(SceneChangedEvent(scene));
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

		auto project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(filePath))
		{
			if (Project::GetActive())
				CloseProject();

			Project::SetActive(project);
			ResourceManager::Init();

			ScriptEngine::LoadAssemblies(Project::ScriptModulePath());

			const auto& config = Project::GetActive()->GetConfig();
			if (!LoadScene(config.ProjectDirectory / config.StartupScenePath))
				NewScene();

			FileSystem::StartWatching(Project::AssetsPath());
			DistributeEvent(ProjectChangedEvnet(project));

			m_ProjectEditData = config;
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

		// Note(moro): When CloseProject gets called durring application shutdown the application cant Raise event anymore
		//             to get around this issue the event is distributed internal
		if (!Application::Get().CanRaiseEvents())
			OnEvent(SceneChangedEvent(nullptr));
		ScriptEngine::UnloadAssemblies();
		ScriptEngine::SetActiveScene(nullptr);

		SK_CORE_ASSERT(m_WorkScene->GetRefCount() == 1);
		m_WorkScene = nullptr;

		FileSystem::StopWatching();

		Project::SetActive(nullptr);
		DistributeEvent(ProjectChangedEvnet(nullptr));
	}

	void EditorLayer::SaveProject()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(Project::GetActive());

		auto& config = Project::GetActive()->GetConfig();
		SaveProject(config.ProjectDirectory / "Project.skproj");
	}

	void EditorLayer::SaveProject(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(Project::GetActive());

		ProjectSerializer serializer(Project::GetActive());
		serializer.Serialize(filePath);
	}

	glm::mat4 EditorLayer::GetViewProjFromCameraEntity(Entity cameraEntity)
	{
		SK_PROFILE_FUNCTION();

		auto& camera = cameraEntity.GetComponent<CameraComponent>();
		auto& tf = cameraEntity.Transform();
		return camera.GetProjection() * glm::inverse(tf.CalcTransform());
	}

	void EditorLayer::DistributeEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		Application::Get().OnEvent(event);
	}

	void EditorLayer::UpdateScriptComponents()
	{
		SK_PROFILE_FUNCTION();

		auto view = m_ActiveScene->GetAllEntitysWith<ScriptComponent>();
		for (auto entityID : view)
		{
			Entity entity = { entityID, m_ActiveScene };
			auto& comp = view.get<ScriptComponent>(entityID);
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(comp.ScriptName);
			ScriptEngine::SetScriptClass(entity, klass);
		}

	}

	void EditorLayer::RunScriptSetup()
	{
		ExecuteSpecs specs;
		specs.Target = fmt::format(L"{}/Premake/premake5.exe", Project::Directory());
		// vs2022 dosn't work for some reason but vs2019 still generates vs2022 solution
		auto sharkDir = std::filesystem::current_path().parent_path();
		specs.Params = L"vs2019";
		specs.WaitUntilFinished = true;
		specs.WorkingDirectory = Project::Directory();
		specs.InterhitConsole = true;
		PlatformUtils::Execute(specs);
	}

	void EditorLayer::OpenIDE()
	{
		SK_PROFILE_FUNCTION();

		RunScriptSetup();
		auto solutionPath = fmt::format("{}/{}.sln", Project::Directory(), Project::Name());
		PlatformUtils::Execute(ExectueVerb::Open, solutionPath);
	}

}
