#include "skfpch.h"
#include "EditorLayer.h"

#include <Shark/Scene/Components.h>

#include "Shark/Asset/SceneSerialization.h"
#include <Shark/Utility/PlatformUtils.h>
#include <Shark/UI/UI.h>

#include "Shark/File/FileWatcher.h"
#include "Shark/Core/Project.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Debug/Instrumentor.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>


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
		
		// NOTE(moro): These values are not correct but are needed to for initialization
		//             The Viewport Size is correct after Resizing in the first frame.
		auto& window = Application::Get().GetWindow();
		m_ViewportWidth = window.GetWidth();
		m_ViewportHeight = window.GetHeight();
		m_EditorCamera.SetProjection(16.0f / 9.0f, 45, 0.01f, 1000.0f);


		// Load Project
		if (!m_StartupProject.empty())
			OpenProject(m_StartupProject);
		else
			SK_CORE_ASSERT(false, "No Startup Project!");

		SK_CORE_ASSERT(m_NextActiveScene, "Failed to load Startup Scene from Project");
		m_ActiveScene = m_NextActiveScene;
		m_WorkScene = m_NextActiveScene;
		m_NextActiveScene = nullptr;

		m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);


		// Create and setup Panels
		m_ContentBrowserPanel = Scope<ContentBrowserPanel>::Create();
		m_SceneHirachyPanel = Scope<SceneHirachyPanel>::Create();
		m_SceneHirachyPanel->SetContext(m_ActiveScene);
		m_SceneHirachyPanel->SetSelectionChangedCallback([this](Entity entity) { m_SelectetEntity = entity; });

		SK_CORE_ASSERT(FileWatcher::IsRunning());


		// Load icons
		m_PlayIcon       = Texture2D::Create("Resources/PlayButton.png");
		m_StopIcon       = Texture2D::Create("Resources/StopButton.png");
		m_SimulateIcon   = Texture2D::Create("Resources/SimulateButton.png");
		m_PauseIcon      = Texture2D::Create("Resources/PauseButton.png");
		m_StepIcon       = Texture2D::Create("Resources/StepButton.png");


		// Renderer stuff
		m_SceneRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);
		m_CameraPreviewRenderer = Ref<SceneRenderer>::Create(m_ActiveScene);

		m_DebugRenderer = Ref<Renderer2D>::Create(m_SceneRenderer->GetExternalCompositFrameBuffer());

		ImageSpecification imageSpecs = m_SceneRenderer->GetIDImage()->GetSpecification();
		imageSpecs.Type = ImageType::Storage;
		m_MousePickingImage = Image2D::Create(imageSpecs, nullptr);

		FileWatcher::SetFileChangedCallback(std::bind(&EditorLayer::OnFileChanged, this, std::placeholders::_1));
	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		CloseProject();
		FileWatcher::SetFileChangedCallback(nullptr);
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (m_NextActiveScene)
		{
			if (m_ActiveScene && ResourceManager::IsMemoryAsset(m_ActiveScene->Handle))
				ResourceManager::UnloadAsset(m_ActiveScene->Handle);

			m_WorkScene = m_NextActiveScene;
			SetActiveScene(m_NextActiveScene);
			m_NextActiveScene = nullptr;
		}

		m_TimeStep = ts;

		Renderer::NewFrame();

		Application::Get().GetImGuiLayer().BlockEvents(!m_ViewportHovered);

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

			if (m_ViewportHovered && m_SceneState != SceneState::Play)
				m_EditorCamera.OnUpdate(ts);

			const glm::mat4 viewProj = GetActiveViewProjection();

			switch (m_SceneState)
			{
				case SceneState::Edit:
				{
					m_ActiveScene->OnUpdateEditor(ts);
					m_ActiveScene->OnRender(m_SceneRenderer, viewProj);
					break;
				}
				case SceneState::Play:
				{
					m_ActiveScene->OnUpdateRuntime(ts);
					m_ActiveScene->OnRender(m_SceneRenderer, viewProj);
					break;
				}
				case SceneState::Simulate:
				{
					m_ActiveScene->OnSimulate(ts);
					m_ActiveScene->OnRender(m_SceneRenderer, viewProj);
					break;
				}
				case SceneState::Pause:
				{
					if (m_UpdateNextFrame)
					{
						if (m_InitialSceneState == SceneState::Play)
							m_ActiveScene->OnUpdateRuntime(1.0f / 60.0f);
						else if (m_InitialSceneState == SceneState::Simulate)
							m_ActiveScene->OnSimulate(1.0f / 60.0f);

						m_UpdateNextFrame = false;
					}

					m_ActiveScene->OnRender(m_SceneRenderer, viewProj);
					break;
				}
			}

			RenderCameraPreview();
			DebugRender();
		}

		Renderer::GetRendererAPI()->BindMainFrameBuffer();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowResize));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));

		if (m_SceneState != SceneState::Play)
			m_EditorCamera.OnEvent(event);
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& event)
	{
		SK_PROFILE_FUNCTION();

		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		Renderer::GetRendererAPI()->ResizeSwapChain(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		SK_PROFILE_FUNCTION();

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
					SK_CORE_ASSERT(true);
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
					const auto& tf = m_SelectetEntity.GetTransform();
					m_EditorCamera.SetFocusPoint(tf.Position);
					return true;
				}
				break;
			}

			case Key::Entf:
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
			case Key::Q: { m_CurrentOperation = 0; return true; }
			case Key::W: { m_CurrentOperation = ImGuizmo::TRANSLATE; return true; }
			case Key::E: { m_CurrentOperation = ImGuizmo::ROTATE; return true; }
			case Key::R: { m_CurrentOperation = ImGuizmo::SCALE; return true; }
		}

		return false;
	}

	void EditorLayer::OnFileChanged(const std::vector<FileChangedData>& fileEvents)
	{
		m_ContentBrowserPanel->OnFileChanged(fileEvents);
	}

	void EditorLayer::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
										          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
										          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImVec2 size = ImGui::GetContentRegionAvail();

		m_ViewportSizeChanged = false;
		if (m_ViewportWidth != size.x || m_ViewportHeight != size.y)
		{
			m_ViewportWidth = (uint32_t)size.x;
			m_ViewportHeight = (uint32_t)size.y;
			m_ViewportSizeChanged = true;
			m_NeedsResize = true;
		}

		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalImage();

		UI::SetBlend(false);
		ImGui::Image(fbImage->GetViewID(), size);
		UI::SetBlend(true);

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		UI_Gizmo();

		// TODO(moro): Mouse Picking with Raycast
#if 1
		// Mouse Picking
		if (!ImGuizmo::IsUsing())
		{
			SK_PROFILE_SCOPED("EditorLayer::OnImGuiRender Mouse Picking");
			SK_PERF_SCOPED("Mouse Picking");
			// TODO(moro): Move to EditorLayer::OnUpdate()

			auto [mx, my] = ImGui::GetMousePos();
			auto [wx, wy] = window->WorkRect.Min;
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
					idImage->CopyTo(m_MousePickingImage);

					m_MousePickingImage->ReadPixel(x, y, (uint32_t&)m_HoveredEntityID);
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
		}
#endif

		if (m_ImportAssetData.Active)
		{
			ImGui::Begin("Import Asset");

			UI::Text("Importing Asset");
			UI::Text(fmt::format("Source: {}", m_ImportAssetData.SourceFile));
			UI::Text(fmt::format("Type: {}", AssetTypeToString(AssetExtentionMap[m_ImportAssetData.Extention])));

			UI::Text(m_ImportAssetData.TargetDirectory);
			ImGui::SameLine(0.0f, 0.0f);
			UI::Text("/");
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::InputText("##ImputAssetName", &m_ImportAssetData.FileName);

			std::filesystem::path fullPath = Project::GetProjectDirectory() / m_ImportAssetData.TargetDirectory / m_ImportAssetData.FileName;
			fullPath.replace_extension(m_ImportAssetData.Extention);

			if (m_ImportAssetData.FileName.empty() || FileSystem::Exists(fullPath))
			{
				auto& style = ImGui::GetStyle();
				UI::Text("Import", UI::Flags::Text_Aligned | UI::Flags::Text_Disabled);
			}
			else
			{
				if (ImGui::Button("Import"))
				{
					std::error_code errorCode;
					if (std::filesystem::copy_file(m_ImportAssetData.SourceFile, fullPath, errorCode))
					{
						ResourceManager::ImportAsset(fullPath);
						m_ImportAssetData.Active = false;
					}

					if (errorCode)
					{
						SK_CORE_ERROR("Failed to copy file! {}", errorCode.message());
					}

				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancle"))
				m_ImportAssetData.Active = false;

			ImGui::End();
		}

		UI_DragDrop();

		// End Viewport
		ImGui::End();

		UI_Info();
		UI_Shaders();
		UI_EditorCamera();
		UI_ToolBar();
		UI_Settings();
		UI_CameraPrevie();
		UI_Stats();
		UI_ProjectSettings();
		UI_Asset();

		m_SceneHirachyPanel->OnImGuiRender(m_ShowSceneHirachyPanel);
		m_ContentBrowserPanel->OnImGuiRender(m_ShowAssetsPanel);

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

				if (ImGui::MenuItem("Create Project"))
					CreateProject();

				ImGui::Separator();
				if (ImGui::MenuItem("Import Asset"))
					ImportAsset();

				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					Application::Get().CloseApplication();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Scene Hirachy", nullptr, &m_ShowSceneHirachyPanel);
				ImGui::MenuItem("Content Browser", nullptr, &m_ShowAssetsPanel);
				ImGui::Separator();
				ImGui::MenuItem("Project", nullptr, &m_ShowProjectSettings);
				ImGui::MenuItem("Shaders", nullptr, &m_ShowShaders);
				ImGui::MenuItem("AssetRegistry", nullptr, &m_ShowAssetsRegistry);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::MenuItem("Editor Camera", nullptr, &m_ShowEditorCameraControlls);
				ImGui::MenuItem("Info", nullptr, &m_ShowInfo);
				ImGui::MenuItem("Stats", nullptr, &m_ShowStats);
				ImGui::MenuItem("ImGui Demo Window", nullptr, &s_ShowDemoWindow);

				ImGui::Separator();


				ImGui::EndMenu();
			}


			ImGui::EndMainMenuBar();
		}
	}

	void EditorLayer::UI_Gizmo()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_CurrentOperation != 0 && m_SelectetEntity)
		{
			SK_CORE_ASSERT(m_SelectetEntity.HasComponent<TransformComponent>(), "Every entity is requiert to have a Transform Component");

			ImGuiWindow* window = ImGui::GetCurrentWindow();

			ImVec2 windowPos = window->WorkRect.Min;
			ImVec2 windowSize = window->WorkRect.GetSize();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);


			glm::mat4 view;
			glm::mat4 projection;
			if (m_SceneState == SceneState::Play)
			{
				Entity cameraEntity = m_ActiveScene->FindActiveCameraEntity();
				SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
				auto& transform = cameraEntity.GetTransform();

				ImGuizmo::SetOrthographic(camera.GetProjectionType() == SceneCamera::Projection::Orthographic);
				view = glm::inverse(transform.GetTranform());
				projection = camera.GetProjection();
			}
			else
			{
				ImGuizmo::SetOrthographic(false);
				view = m_EditorCamera.GetView();
				projection = m_EditorCamera.GetProjection();
			}

			auto& tf = m_SelectetEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tf.GetTranform();

			float* snap = nullptr;
			if (Input::KeyPressed(Key::LeftShift))
			{
				ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)m_CurrentOperation;
				switch (operation)
				{
					case ImGuizmo::TRANSLATE: snap = &m_TranslationSnap; break;
					case ImGuizmo::ROTATE:    snap = &m_RotationSnap; break;
					case ImGuizmo::SCALE:     snap = &m_ScaleSnap; break;
				}
			}

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), (ImGuizmo::OPERATION)m_CurrentOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(transform), nullptr, snap);

			if (!Input::KeyPressed(Key::Alt) && ImGuizmo::IsUsing())
			{
				switch (m_CurrentOperation)
				{
					case ImGuizmo::TRANSLATE: Math::DecomposeTranslation(transform, tf.Position); break;
					case ImGuizmo::ROTATE:    glm::extractEulerAngleXYZ(transform, tf.Rotation.x, tf.Rotation.y, tf.Rotation.z); break;
					case ImGuizmo::SCALE:     Math::DecomposeScale(transform, tf.Scaling); break;
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

			ImGui::NewLine();
			static MemoryMetrics s_LastMemory;
			const auto& m = MemoryManager::GetMetrics();
			ImGui::Text("Memory Usage: %llu", m.MemoryUsage());
			ImGui::Text("Memory Allocated: %llu", m.MemoryAllocated);
			ImGui::Text("Memory Freed: %llu", m.MemoryFreed);
			ImGui::Text("Total Count: %llu", m.TotalAllocated - m.TotalFreed);
			ImGui::Text("Total Allocated: %llu", m.TotalAllocated);
			ImGui::Text("Total Freed: %llu", m.TotalFreed);

			ImGui::NewLine();

			ImGui::Text("Memory Usage Delta: %llu", (m.MemoryAllocated - s_LastMemory.MemoryAllocated) - (m.MemoryFreed - s_LastMemory.MemoryFreed));
			ImGui::Text("Memory Allocated Delta: %llu", m.MemoryAllocated - s_LastMemory.MemoryAllocated);
			ImGui::Text("Memory Freed Delta: %llu", m.MemoryFreed - s_LastMemory.MemoryFreed);
			ImGui::Text("Total Count Delta: %llu", (m.TotalAllocated - s_LastMemory.TotalAllocated) - (m.TotalFreed - s_LastMemory.TotalFreed));
			ImGui::Text("Total Allocated Delta: %llu", m.TotalAllocated - s_LastMemory.TotalAllocated);
			ImGui::Text("Total Freed Delta: %llu", m.TotalFreed - s_LastMemory.TotalFreed);
			s_LastMemory = m;

			ImGui::Checkbox("Negative Effect", &m_NegativeEffect);
			ImGui::Checkbox("Blur Effect", &m_BlurEffect);

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
				UI::BeginProperty();

				//UI::DragFloat("Position", m_EditorCamera.GetPosition());

				auto focuspoint = m_EditorCamera.GetFocusPoint();
				if (UI::DragFloat("FocusPoint", focuspoint))
					m_EditorCamera.SetFocusPoint(focuspoint);

				glm::vec2 py = { m_EditorCamera.GetPitch(), m_EditorCamera.GetYaw() };
				if (UI::DragFloat("Orientation", py))
				{
					m_EditorCamera.SetPicht(py.x);
					m_EditorCamera.SetYaw(py.y);
				}

				float distance = m_EditorCamera.GetDistance();
				if (UI::DragFloat("Distance", distance, 10))
					if (distance >= 0.25f)
						m_EditorCamera.SetDistance(distance);

				if (ImGui::Button("Reset"))
				{
					m_EditorCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
					m_EditorCamera.SetPicht(0.0f);
					m_EditorCamera.SetYaw(0.0f);
					m_EditorCamera.SetDistance(10.0f);
				}

				UI::EndProperty();
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
								Ref<Scene> scene = ResourceManager::GetAsset<Scene>(handle);
								SK_CORE_ASSERT(scene);
								SetNextActiveScene(scene);
								break;
							}
							case AssetType::Texture:
							{
								Entity entity = m_ActiveScene->CreateEntity();
								auto& sr = entity.AddComponent<SpriteRendererComponent>();
								sr.TextureHandle = handle;
								SelectEntity(entity);
							}
						}
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
		UI::MoveCursorX(ImGui::GetWindowContentRegionWidth() * 0.5f - (size * 3.0f * 0.5f) - (style.ItemSpacing.x * 3.0f));

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


		const auto imageButton = [this, size](auto strid, auto texture) { return ImGui::ImageButtonEx(UI::GetID(strid), texture->GetViewID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0, 0 }, { 0, 0, 0, 0 }, { 1, 1, 1, 1 }); };
		const auto imageButtonDisabled = [this, size](auto strid, auto texture) { ImGui::Image(texture->GetViewID(), { size, size }, { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f, 0.5f, 1.0f }); };

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				// [Play]
				if (imageButton("PlayIcon", m_PlayIcon))
					OnScenePlay();

				// [Simulate]
				ImGui::SameLine();
				if (imageButton("SimulateIcon", m_SimulateIcon))
					OnSimulateStart();

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", m_StepIcon);

				break;
			}
			case SceneState::Play:
			{
				// [Stop]
				if (imageButton("StopIcon", m_StopIcon))
					OnSceneStop();

				// [Pause]
				ImGui::SameLine();
				if (imageButton("Pause", m_PauseIcon))
					m_SceneState = SceneState::Pause;

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", m_StepIcon);

				break;
			}
			case SceneState::Simulate:
			{
				// [Stop]
				if (imageButton("StopIcon", m_StopIcon))
					OnSceneStop();

				// [Pause]
				ImGui::SameLine();
				if (imageButton("Pause", m_PauseIcon))
					m_SceneState = SceneState::Pause;

				// [Step Disabled]
				ImGui::SameLine();
				imageButtonDisabled("Step Disabled", m_StepIcon);

				break;
			}
			case SceneState::Pause:
			{
				// [Stop]
				if (imageButton("StopIcon", m_StopIcon))
					OnSceneStop();

				// [UnPause]
				ImGui::SameLine();
				if (imageButton("UnPause", m_PlayIcon))
					m_SceneState = m_InitialSceneState;

				// [Step]
				ImGui::SameLine();
				if (imageButton("Step", m_StepIcon))
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
						UI::BeginPropertyGrid();
						UI::Checkbox("Camera Preview", m_ShowCameraPreview);
						UI::EndProperty();
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("Colliders", UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::BeginPropertyGrid();
						UI::Checkbox("Show", m_ShowColliders);
						UI::Checkbox("OnTop", m_ShowCollidersOnTop);
						UI::EndProperty();
						ImGui::TreePop();
					}
				}

				if (ImGui::CollapsingHeader("Stuff"))
				{
					UI::BeginPropertyGrid();
					auto& window = Application::Get().GetWindow();
					bool vSync = window.IsVSync();
					if (UI::Checkbox("VSync", vSync))
						window.SetVSync(vSync);

					UI::Checkbox("Read Hoved Entity", m_ReadHoveredEntity);
					UI::EndProperty();
				}
			}
			ImGui::End();
		}
	}

	static void CameraPreviewResizeCallback(ImGuiSizeCallbackData* data)
	{
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
		UI::Text(fmt::format("Present GPU: {:.4f}ms", Renderer::GetPresentTimer()->GetTime().MilliSeconds()));
		UI::Text(fmt::format("ImGuiLayer GPU: {:.4f}ms", ProfilerRegistry::GetAverageOf("[GPU] DirectXImGuiLayer::End").MilliSeconds()));
		ImGui::End();


		ImGui::Begin("CPU Times");
		UI::Text(fmt::format("FrameTime: {:.4f}ms", m_TimeStep.MilliSeconds()));
		UI::Text(fmt::format("Present CPU: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::Present").MilliSeconds()));
		UI::Text(fmt::format("NewFrame: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::NewFrame").MilliSeconds()));
		UI::Text(fmt::format("Renderer2D EndScene: {:.4f}ms", ProfilerRegistry::GetAverageOf("Renderer2D::EndScene").MilliSeconds()));
		UI::Text(fmt::format("RenderGeometry: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderGeometry").MilliSeconds()));
		UI::Text(fmt::format("RenderFullScreenQuad: {:.4f}ms", ProfilerRegistry::GetAverageOf("DirectXRenderer::RenderFullScreenQuad").MilliSeconds()));
		ImGui::End();

	}

	void EditorLayer::UI_ProjectSettings()
	{
		if (!m_ShowProjectSettings)
			return;

		ImGui::Begin("Project", &m_ShowProjectSettings);

		auto& config = Project::GetActive()->GetConfig();
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

		UI::BeginPropertyGrid();

		if (UI::PropertyCustom("Name"))
		{
			UI::SpanAvailWith();
			ImGui::InputText("##Name", &config.Name);
		}

		if (UI::PropertyCustom("Assets"))
		{
			UI::PushID("Assets");

			UI::ScopedStyle style;
			if (!m_ProjectEditData.ValidAssetsPath)
				style.Push(ImGuiCol_Text, UI::Theme::GetColors().TextInvalidInput);

			UI::SpanAvailWith();
			if (ImGui::InputText("##Assets", &m_ProjectEditData.Assets))
			{
				auto assetsDirectory = Project::AbsolueCopy(m_ProjectEditData.Assets);
				m_ProjectEditData.ValidAssetsPath = (std::filesystem::exists(assetsDirectory) && std::filesystem::is_directory(assetsDirectory));
			}

			UI::PopID();
		}

		if (UI::PropertyCustom("StartupScene"))
		{
			UI::PushID("StartupScene");

			UI::ScopedStyle style;
			if (!m_ProjectEditData.ValidStartupScene)
				style.Push(ImGuiCol_Text, UI::Theme::GetColors().TextInvalidInput);

			UI::SpanAvailWith();
			if (ImGui::InputText("##StartupScene", &m_ProjectEditData.StartupScene))
			{
				auto startupScene = Project::AbsolueCopy(m_ProjectEditData.StartupScene);
				m_ProjectEditData.ValidStartupScene = std::filesystem::exists(startupScene) && std::filesystem::is_regular_file(startupScene) && (startupScene.extension() == L".skscene");
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
						m_ProjectEditData.StartupScene = Project::RelativeCopy(ResourceManager::GetFileSystemPath(metadata)).string();
						m_ProjectEditData.ValidStartupScene = true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			UI::PopID();
		}

		UI::DragFloat("Gravity", config.Gravity, 0.0f, 0.0f, 0.0f, 0.1f, "%f");
		UI::DragInt("Velocity Iterations", config.VelocityIterations, 8, 1, INT_MAX);
		UI::DragInt("Position Iterations", config.PositionIterations, 3 , 1, INT_MAX);
		float fixedTSInMS = config.FixedTimeStep * 1000.0f;
		if (UI::DragFloat("Fixed Time Step", fixedTSInMS, 1.0f, 0.1f, FLT_MAX, 1.0f, "%fms"))
			config.FixedTimeStep = fixedTSInMS * 0.001f;

		UI::EndProperty();

		ImGui::End();
	}

	void EditorLayer::UI_Asset()
	{
		if (!m_ShowAssetsRegistry)
			return;

		ImGui::Begin("Asset Registry", &m_ShowAssetsRegistry);

		static std::string SearchBuffer;
		UI::SpanAvailWith();
		ImGui::InputText("##SearchBuffer", &SearchBuffer);

		if (ImGui::TreeNodeEx("Asset Registry", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen, "Asset Registry %d", ResourceManager::GetAssetRegistry().Count()))
		{
			for (auto [handle, metadata] : ResourceManager::GetAssetRegistry())
			{
				if (!SearchBuffer.empty())
				{
					const std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					const std::string typeStr = AssetTypeToString(metadata.Type);
					const std::string filePathStr = metadata.FilePath.string();

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= filePathStr.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}

				UI::BeginPropertyGrid();
				UI::PushTextFlag(UI::Flags::Text_Aligned);

				UI::Property("Handle", fmt::format("{:x}", metadata.Handle), UI::Flags::Text_Selectable);
				UI::Property("FilePath", metadata.FilePath, UI::Flags::Text_Selectable);
				UI::Property("Type", AssetTypeToString(metadata.Type), UI::Flags::Text_Selectable);

				UI::PopTextFlag();
				UI::EndProperty();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Loaded Assets", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected, "Loaded Assets %d", ResourceManager::GetLoadedAssets().size()))
		{
			for (auto [handle, asset] : ResourceManager::GetLoadedAssets())
			{
				const auto& metadata = ResourceManager::GetMetaData(asset);

				if (!SearchBuffer.empty())
				{
					const std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					const std::string typeStr = AssetTypeToString(metadata.Type);
					const std::string filePathStr = metadata.FilePath.string();

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= filePathStr.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}

				UI::BeginPropertyGrid();
				UI::PushTextFlag(UI::Flags::Text_Aligned);

				UI::Property("Handle", fmt::format("{:x}", metadata.Handle), UI::Flags::Text_Selectable);
				UI::Property("FilePath", metadata.FilePath, UI::Flags::Text_Selectable);
				UI::Property("Type", AssetTypeToString(metadata.Type), UI::Flags::Text_Selectable);

				UI::PopTextFlag();
				UI::EndProperty();
			}

			ImGui::TreePop();
		}
		
		if (ImGui::TreeNodeEx("Memory Assets", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected, "Memory Assets %d", ResourceManager::GetMemoryAssets().size()))
		{
			for (auto [handle, asset] : ResourceManager::GetMemoryAssets())
			{
				if (!SearchBuffer.empty())
				{
					const std::string handleStrHex = fmt::format("{:x}", handle);
					const std::string typeStr = AssetTypeToString(asset->GetAssetType());

					bool matchFound = false;

					matchFound |= handleStrHex.find(SearchBuffer) != std::string::npos;
					matchFound |= typeStr.find(SearchBuffer) != std::string::npos;

					if (!matchFound)
						continue;
				}

				UI::BeginPropertyGrid();
				UI::PushTextFlag(UI::Flags::Text_Aligned);

				UI::Property("Handle", fmt::format("{:x}", handle), UI::Flags::Text_Selectable);
				UI::Property("Type", AssetTypeToString(asset->GetAssetType()), UI::Flags::Text_Selectable);

				UI::PopTextFlag();
				UI::EndProperty();
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}

	void EditorLayer::DebugRender()
	{
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
						auto& tf = entity.GetTransform();

						glm::mat4 transform =
							tf.GetTranform() *
							glm::translate(glm::mat4(1), glm::vec3(collider.Offset, 0)) *
							glm::eulerAngleZ(collider.Rotation) *
							glm::scale(glm::mat4(1), glm::vec3(collider.Size * 2.0f, 1.0f));
						
						m_DebugRenderer->DrawRectOnTop(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
					}
				}

				{
					auto view = m_ActiveScene->GetAllEntitysWith<CircleCollider2DComponent>();
					for (auto entityID : view)
					{
						Entity entity{ entityID, m_ActiveScene };
						auto& collider = view.get<CircleCollider2DComponent>(entityID);
						auto& tf = entity.GetTransform();

						glm::mat4 transform =
							tf.GetTranform() *
							glm::translate(glm::mat4(1), glm::vec3(collider.Offset, 0)) *
							glm::eulerAngleZ(collider.Rotation) *
							glm::scale(glm::mat4(1), glm::vec3(collider.Radius, collider.Radius, 1.0f));

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
						auto& tf = entity.GetTransform();

						glm::mat4 transform =
							tf.GetTranform() *
							glm::translate(glm::mat4(1), glm::vec3(collider.Offset, 0)) *
							glm::eulerAngleZ(collider.Rotation) *
							glm::scale(glm::mat4(1), glm::vec3(collider.Size * 2.0f, 1.0f));

						m_DebugRenderer->DrawRect(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
					}
				}

				{
					auto view = m_ActiveScene->GetAllEntitysWith<CircleCollider2DComponent>();
					for (auto entityID : view)
					{
						Entity entity{ entityID, m_ActiveScene };
						auto& collider = view.get<CircleCollider2DComponent>(entityID);
						auto& tf = entity.GetTransform();

						glm::mat4 transform =
							tf.GetTranform() *
							glm::translate(glm::mat4(1), glm::vec3(collider.Offset, 0)) *
							glm::eulerAngleZ(collider.Rotation) *
							glm::scale(glm::mat4(1), glm::vec3(0.0f, 0.0f, collider.Radius));

						m_DebugRenderer->DrawCircle(transform, { 0.1f, 0.3f, 0.9f, 1.0f });
					}
				}
			}

			m_DebugRenderer->EndScene();
		}
	}

	void EditorLayer::RenderCameraPreview()
	{
		if (m_ShowCameraPreview)
		{
			Entity cameraEntity = m_ActiveScene->FindActiveCameraEntity();
			if (cameraEntity)
				m_ActiveScene->OnRender(m_CameraPreviewRenderer, GetViewProjFromCameraEntity(cameraEntity));
		}
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
		m_SelectetEntity = entity;
		m_SceneHirachyPanel->SetSelectedEntity(entity);
	}

	glm::mat4 EditorLayer::GetActiveViewProjection() const
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity cameraEntity = m_ActiveScene->GetRuntimeCamera();
			auto& camera = cameraEntity.GetComponent<CameraComponent>();
			auto& tf = cameraEntity.GetTransform();
			return camera.GetProjection() * glm::inverse(tf.GetTranform());
		}

		return m_EditorCamera.GetViewProjection();
	}

	void EditorLayer::NewScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		SetNextActiveScene(ResourceManager::CreateMemoryAsset<Scene>());
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
			SetNextActiveScene(scene);
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
			auto filePath = FileDialogs::SaveFile(L"|*.*|Shark|*.skscene", 2, Project::GetAssetsPath(), true);

			std::string directoryPath = ResourceManager::GetRelativePathString(filePath.parent_path());
			std::string fileName = filePath.filename().string();
			return ResourceManager::AddMemoryAssetToRegistry(m_ActiveScene->Handle, directoryPath, fileName);
		}

		SK_CORE_ASSERT(false, "Normal assets currently cant be saved at a different location");
		return false;

#if 0
		auto filepath = FileDialogs::SaveFile(L"|*.*|Shark|*.skscene", 2, Project::GetAssetsPathAbsolute(), true);
		if (!filepath.empty())
			return SerializeScene(m_ActiveScene, filepath);
		return false;
#endif
	}

	bool EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(false, "Not Implemented");
		return false;
#if 0
		SK_PROFILE_FUNCTION();
		
		SceneSerializer serializer(scene);
		return serializer.Serialize(filePath);
#endif
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		m_InitialSceneState = SceneState::Play;
		SetActiveScene(Scene::Copy(m_WorkScene));
		m_ActiveScene->OnScenePlay();
		m_SceneHirachyPanel->ScenePlaying(true);
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();
		
		m_ActiveScene->OnSceneStop();
		SetActiveScene(m_WorkScene);

		m_SceneHirachyPanel->ScenePlaying(false);
		m_SceneState = SceneState::Edit;
		m_InitialSceneState = SceneState::None;
		m_SceneRenderer->SetScene(m_WorkScene);

		if (!m_ActiveScene->IsValidEntity(m_SceneHirachyPanel->GetSelectedEntity()))
			SelectEntity({});
	}

	void EditorLayer::OnSimulateStart()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Simulate;
		m_InitialSceneState = SceneState::Simulate;
		SetActiveScene(Scene::Copy(m_WorkScene));

		m_ActiveScene->OnSimulateStart();
		m_SceneHirachyPanel->ScenePlaying(true);
	}

	void EditorLayer::SetActiveScene(const Ref<Scene>& scene)
	{
		SK_PROFILE_FUNCTION();
		
		m_ActiveScene = scene;
		m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		m_SceneHirachyPanel->SetContext(m_ActiveScene);
		m_SceneRenderer->SetScene(m_ActiveScene);
		m_CameraPreviewRenderer->SetScene(m_ActiveScene);
	}

	void EditorLayer::SetNextActiveScene(const Ref<Scene>& scene)
	{
		m_NextActiveScene = scene;
	}

	void EditorLayer::OpenProject()
	{
		auto filePath = FileDialogs::OpenFile(L"|*.*|Project|*.skproj", 2);
		if (!filePath.empty())
			OpenProject(filePath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& filePath)
	{
		SK_CORE_INFO("Opening Project [{}]", filePath);

		auto project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(filePath))
		{
			if (Project::GetActive())
				CloseProject();

			Project::SetActive(project);
			ResourceManager::Init();

			const auto& config = Project::GetActive()->GetConfig();
			if (!LoadScene(config.ProjectDirectory / config.StartupScenePath))
				NewScene();

			if (m_ContentBrowserPanel)
				m_ContentBrowserPanel->OnProjectChanged();
			FileWatcher::StartWatching(Project::GetAssetsPath());
			m_ProjectEditData = config;
		}
	}

	void EditorLayer::CloseProject()
	{
		SK_CORE_ASSERT(Project::GetActive());

		SaveProject();

		SK_CORE_INFO("Closing Project");

		m_NextActiveScene = nullptr;
		m_ActiveScene = nullptr;
		m_SceneHirachyPanel->SetContext(nullptr);
		m_SceneRenderer->SetScene(nullptr);
		m_CameraPreviewRenderer->SetScene(nullptr);
		ResourceManager::Shutdown();

		SK_CORE_ASSERT(m_WorkScene->GetRefCount() == 1);
		m_WorkScene = nullptr;

		FileWatcher::StopWatching();

		Project::SetActive(nullptr);
	}

	void EditorLayer::SaveProject()
	{
		SK_CORE_ASSERT(Project::GetActive());

		auto& config = Project::GetActive()->GetConfig();
		SaveProject(config.ProjectDirectory / "Project.skproj");
	}

	void EditorLayer::SaveProject(const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(Project::GetActive());

		ProjectSerializer serializer(Project::GetActive());
		serializer.Serialize(filePath);
	}

	void EditorLayer::CreateProject()
	{
		auto targetDirectory = FileDialogs::OpenDirectory(std::filesystem::current_path());
		SK_CORE_ASSERT(targetDirectory.is_absolute());
		if (targetDirectory.empty())
			return;

		CloseProject();

		SK_CORE_INFO("Creating Project {}", targetDirectory);

		FileSystem::CreateDirectory(targetDirectory / "Assets");
		FileSystem::CreateDirectory(targetDirectory / "Assets/Scenes");
		FileSystem::CreateDirectory(targetDirectory / "Assets/Textures");

		Ref<Project> project = Ref<Project>::Create();
		auto& config = project->GetConfig();
		config.Name = "Untitled";
		config.ProjectDirectory = targetDirectory;
		config.AssetsDirectory = "Assets";
		config.StartupScenePath = "";
		config.Gravity = { 0.0f, 9.81f };
		config.VelocityIterations = 8;
		config.PositionIterations = 3;
		config.FixedTimeStep = 0.001f;

		Project::SetActive(project);
		ResourceManager::Init();
		SaveProject();
		NewScene();

		if (m_ContentBrowserPanel)
			m_ContentBrowserPanel->OnProjectChanged();
		FileWatcher::StartWatching(Project::GetAssetsPath());
	}

	void EditorLayer::ImportAsset()
	{
		m_ImportAssetData = ImportAssetData();

		m_ImportAssetData.SourceFile = FileDialogs::OpenFile(L"|*.*|Scene|.skscene|Texture|.png");
		if (!m_ImportAssetData.SourceFile.empty())
		{
			m_ImportAssetData.Active = true;
			m_ImportAssetData.Extention = m_ImportAssetData.SourceFile.extension().string();
			String::ToLower(m_ImportAssetData.Extention);

			if (!IsValidExtention(m_ImportAssetData.Extention))
			{
				SK_CORE_ERROR("Tryed to Import asset but extention is Unkown! {}", m_ImportAssetData.Extention);
				m_ImportAssetData = ImportAssetData();
				return;
			}

			if (AssetExtentionMap[m_ImportAssetData.Extention] == AssetType::None)
			{
				m_ImportAssetData.Active = false;
				return;
			}

		}

	}

	glm::mat4 EditorLayer::GetViewProjFromCameraEntity(Entity cameraEntity)
	{
		auto& camera = cameraEntity.GetComponent<CameraComponent>();
		auto& tf = cameraEntity.GetTransform();
		return camera.GetProjection() * glm::inverse(tf.GetTranform());
	}

}
