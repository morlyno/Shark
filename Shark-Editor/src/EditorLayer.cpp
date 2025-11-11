#include "EditorLayer.h"

#include "Shark/Core/Project.h"
#include "Shark/Core/SelectionManager.h"

#include "Shark/Scene/Components.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Serialization/ProjectSerializer.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/EditorResources.h"

#include "EditorSettings.h"

#include "Panels/EditorConsolePanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Panels/PhysicsDebugPanel.h"
#include "Panels/ScriptEnginePanel.h"
#include "Panels/AssetsPanel.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/ShadersPanel.h"
#include "Panels/SceneRendererPanel.h"
#include "Panels/StatisticsPanel.h"
#include "Panels/ECSDebugPanel.h"
#include "Panels/IconSelector.h"
#include "Panels/AssetEditorPanel.h"
#include "Panels/Editors/TextureEditorPanel.h"
#include "Panels/Editors/MaterialEditorPanel.h"
#include "Panels/Editors/PrefabEditorPanel.h"

#include "Shark/Debug/Profiler.h"

#include <glm/gtc/type_ptr.hpp>
#include "Shark/Debug/enttDebug.h"

namespace Shark {

	static bool s_ShowDemoWindow = false;

	namespace utils {

		static glm::vec3 GetCenterOfSelections(Ref<Scene> scene, std::span<const UUID> selections)
		{
			glm::vec3 center = glm::vec3(0.0f);
			for (UUID id : selections)
			{
				Entity entity = scene->TryGetEntityByUUID(id);
				glm::vec3 translation;
				Math::DecomposeTranslation(scene->GetWorldSpaceTransformMatrix(entity), translation);
				center += translation;
			}
			center /= selections.size();
			return center;
		}

		template<typename TContainerOrView>
		static glm::vec3 GetCenterOfEntities(Ref<Scene> scene, const TContainerOrView& selectedEntities)
		{
			glm::vec3 center = glm::vec3(0.0f);
			for (Entity entity : selectedEntities)
			{
				glm::vec3 translation;
				Math::DecomposeTranslation(scene->GetWorldSpaceTransformMatrix(entity), translation);
				center += translation;
			}
			center /= selectedEntities.size();
			return center;
		}

	}

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

		Window& window = Application::Get().GetWindow();
		window.SetTitlebarHitTestCallback([this](int x, int y, bool& outHit) { outHit = m_TitlebarHovered; });

		EditorResources::Init();
		EditorSettings::Initialize();

		m_PanelManager = Scope<PanelManager>::Create();
		m_PanelManager->LoadSettings();

		Ref<SceneHierarchyPanel> sceneHirachy = m_PanelManager->AddPanel<SceneHierarchyPanel>(PanelCategory::View, "Scene Hierarchy", true);
		sceneHirachy->RegisterSnapToEditorCameraCallback([this](Entity entity) { entity.Transform().SetTransform(glm::inverse(m_EditorCamera.GetView())); });

		Ref<ContentBrowserPanel> contentBrowser = m_PanelManager->AddPanel<ContentBrowserPanel>(PanelCategory::View, "Content Browser", true);
		contentBrowser->RegisterAssetActicatedCallback(AssetType::Material, [this](const AssetMetaData& metadata)
		{
			m_PanelManager->ShowPanel<AssetEditorManagerPanel>();
			auto assetEditorManager = m_PanelManager->GetPanel<AssetEditorManagerPanel>();
			assetEditorManager->AddEditor<MaterialEditorPanel>(metadata);
		});

		contentBrowser->RegisterAssetActicatedCallback(AssetType::Texture, [this](const AssetMetaData& metadata)
		{
			m_PanelManager->ShowPanel<AssetEditorManagerPanel>();
			auto assetEditorManager = m_PanelManager->GetPanel<AssetEditorManagerPanel>();
			assetEditorManager->AddEditor<TextureEditorPanel>(metadata);
		});

		contentBrowser->RegisterAssetActicatedCallback(AssetType::Prefab, [this](const AssetMetaData& metadata)
		{
			m_PanelManager->ShowPanel<AssetEditorManagerPanel>();
			auto assetEditorManager = m_PanelManager->GetPanel<AssetEditorManagerPanel>();
			assetEditorManager->AddEditor<PrefabEditorPanel>(metadata);
		});

		m_PanelManager->AddPanel<AssetEditorManagerPanel>(PanelCategory::Edit, "Assets Editor Manager", true);
		m_PanelManager->AddPanel<ProjectSettingsPanel>(PanelCategory::Edit, "Project Settings", false, Project::GetActive());
		m_PanelManager->AddPanel<AssetsPanel>(PanelCategory::View, "Assets", false);
		m_PanelManager->AddPanel<EditorConsolePanel>(PanelCategory::View, "Console", true);
		m_PanelManager->AddPanel<ShadersPanel>(PanelCategory::View, "Shaders", false);
		m_PanelManager->AddPanel<PhysicsDebugPanel>(PanelCategory::View, "Physics Debug", false);
		m_PanelManager->AddPanel<ScriptEnginePanel>(PanelCategory::View, "Script Engine", false);
		m_PanelManager->AddPanel<SceneRendererPanel>(PanelCategory::View, "Scene Renderer", true);
		m_PanelManager->AddPanel<StatisticsPanel>(PanelCategory::View, "Statistics", false);
		m_PanelManager->AddPanel<ECSDebugPanel>(PanelCategory::View, "ECS Debug", false, nullptr);
		m_PanelManager->AddPanel<IconSelector>(PanelCategory::Edit, "Icons Selector", false);
		m_PanelManager->AddPanel<MaterialPanel>(PanelCategory::Edit, "Materials", true);

		m_SceneRenderer = Ref<SceneRenderer>::Create(window.GetWidth(), window.GetHeight(), "Viewport Renderer");
		m_PanelManager->GetPanel<SceneRendererPanel>()->SetRenderer(m_SceneRenderer);

		Renderer2DSpecifications debugRendererSpec;
		debugRendererSpec.UseDepthTesting = true;
		m_DebugRenderer = Ref<Renderer2D>::Create(m_SceneRenderer->GetTargetFramebuffer(), debugRendererSpec);

#if TODO // #Renderer #Disabled reading from images is not yet supported
		// Readable image for Mouse Picking
		ImageSpecification imageSpecs = m_SceneRenderer->GetIDImage()->GetSpecification();
		imageSpecs.Type = ImageType::Storage;
		m_MousePickingImage = Image2D::Create(imageSpecs);
#endif

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
	}

	void EditorLayer::OnDetach()
	{
		SK_PROFILE_FUNCTION();

		m_PanelManager->RemoveAll();

		CloseProject();

		EditorResources::Shutdown();
		EditorSettings::Shutdown();
	}

	void EditorLayer::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_TimeStep = ts;

		ReloadScriptEngineIfNeeded();

		if (m_ActiveScene)
		{
			if (m_NeedsResize && m_ViewportWidth != 0 && m_ViewportHeight != 0)
			{
				SK_PROFILE_SCOPED("EditorLayer::OnUpdate Resize");

				if (m_ActiveScene != m_EditorScene)
					m_EditorScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
				m_ActiveScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
				m_SceneRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
				m_DebugRenderer->Resize(m_ViewportWidth, m_ViewportHeight);
				//m_MousePickingImage->Resize(m_ViewportWidth, m_ViewportHeight); // #Renderer #Disabled
				m_EditorCamera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
				m_NeedsResize = false;
			}

			ImGui::GetIO().SetAppAcceptingEvents(Input::GetCursorMode() != CursorMode::Locked);

			switch (m_SceneState)
			{
				case SceneState::Edit:
				{
					m_EditorCamera.OnUpdate(ts, m_ViewportHovered || m_ViewportFocused && Input::GetCursorMode() == CursorMode::Locked);
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
					m_EditorCamera.OnUpdate(ts, m_ViewportHovered || m_ViewportFocused && Input::GetCursorMode() == CursorMode::Locked);
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

		m_EditorCamera.OnEvent(event);
		m_PanelManager->OnEvent(event);

		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispacher.DispachEvent<WindowDropEvent>(SK_BIND_EVENT_FN(EditorLayer::OnWindowDropEvent));
		dispacher.DispachEvent<AssetReloadedEvent>([this](AssetReloadedEvent& e)
		{
			AssetType assetType = AssetManager::GetAssetType(e.Asset);
			if (assetType == AssetType::Scene && e.Asset == m_EditorScene->Handle)
				m_EditorScene = AssetManager::GetAsset<Scene>(e.Asset);
			return false;
		});
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
			return false;

		if (event.IsRepeat())
			return false;

		const auto& modifiers = event.GetModifierKeys();

		switch (event.GetKeyCode())
		{
			case KeyCode::Escape:
			{
				//Input::SetCursorMode(CursorMode::Normal);
				if (!m_EditorCamera.GetFlyMode())
				{
					SelectionManager::DeselectAll();
					return true;
				}
				break;
			}

			// New Scene
			case KeyCode::N:
			{
				if (modifiers.Control)
				{
					NewScene();
					return true;
				}
				break;
			}

			// Save Scene
			case KeyCode::S:
			{
				if (modifiers.Control)
				{
					if (modifiers.Shift)
						SaveSceneAs();
					else
						SaveScene();
					return true;
				}
				break;
			}

			// Copy Entity
			case KeyCode::D:
			{
				if (!m_ViewportFocused)
					break;

				if (modifiers.Control && SelectionManager::AnySelected(m_EditorScene->GetID()) && m_SceneState == SceneState::Edit)
				{
					auto selections = SelectionManager::GetSelections(m_EditorScene->GetID());
					SelectionManager::DeselectAll(m_EditorScene->GetID());
					for (UUID sourceID : selections)
					{
						Entity source = m_EditorScene->TryGetEntityByUUID(sourceID);
						Entity cloned = m_EditorScene->DuplicateEntity(source);
						SelectionManager::Select(m_EditorScene->GetID(), cloned.GetUUID());
					}

					return true;
				}
				break;
			}

			// Focus Selected Entity
			case KeyCode::F:
			{
				if (!m_ViewportFocused)
					break;

				if (SelectionManager::AnySelected(m_EditorScene->GetID()))
				{
					glm::vec3 center = utils::GetCenterOfSelections(m_ActiveScene, SelectionManager::GetSelections(m_EditorScene->GetID()));

					m_EditorCamera.SetFocusPoint(center);
					m_EditorCamera.SetDistance(7.5f);
					return true;
				}
				break;
			}

			case KeyCode::Delete:
			{
				if (!m_ViewportFocused)
					break;

				if (SelectionManager::AnySelected(m_EditorScene->GetID()))
				{
					for (UUID entityID : SelectionManager::GetSelections(m_EditorScene->GetID()))
					{
						Entity entity = m_EditorScene->TryGetEntityByUUID(entityID);
						if (entity && !entity.HasComponent<MeshFilterComponent>())
						{
							m_EditorScene->DestroyEntity(entity);
						}
					}
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
			case KeyCode::Q:
			{
				if (!m_EditorCamera.GetFlyMode())
				{
					m_CurrentOperation = (ImGuizmo::OPERATION)0;
					return true;
				}
				break;
			}
			case KeyCode::W:
			{
				if (!m_EditorCamera.GetFlyMode())
				{
					m_CurrentOperation = ImGuizmo::TRANSLATE;
					return true;
				}
				break;
			}
			case KeyCode::E:
			{
				if (!m_EditorCamera.GetFlyMode())
				{
					m_CurrentOperation = ImGuizmo::ROTATE;
					return true;
				}
				break;
			}
			case KeyCode::R:
			{
				if (!m_EditorCamera.GetFlyMode())
				{
					m_CurrentOperation = ImGuizmo::SCALE;
					return true;
				}
				break;
			}
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
				Ref<ContentBrowserPanel> panel = m_PanelManager->GetPanel<ContentBrowserPanel>();
				Ref<DirectoryInfo> currentDirectory = panel->GetCurrentDirectory();
				if (currentDirectory)
				{
					auto destination = FileSystem::UniquePath(currentDirectory->Filepath / path.filename());
					FileSystem::CopyFile(path, destination);
				}

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
			EditorResources::ReloadIcons();
			m_ReloadEditorIcons = false;
		}

		UpdateMainWindow();

		const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus /*| ImGuiWindowFlags_MenuBar*/;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

		const bool isMaximized = Application::Get().GetWindow().IsMaximized();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(0.0f, 6.0f) : ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::Colors::Theme::Titlebar);
        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
		ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);

		const bool customTitlebar = Application::Get().GetSpecification().CustomTitlebar;
		if (customTitlebar)
		{
			float titlebarHeight;
			UI_DrawTitleBar(titlebarHeight);
			ImGui::SetCursorPosY(titlebarHeight);
		}

		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		if (!customTitlebar)
			UI_DrawMenuBar();

		UI_Viewport();
		// #Renderer #Disabled #moro mouse picking
		//UI_MousePicking();

		UI_ToolBar();

		UI_EditorCamera();
		UI_OpenProjectModal();
		UI_ImportAsset();
		UI_CreateMeshAsset();
		UI_CreateProjectModal();
		UI_ShowKeyStates();

		m_PanelManager->OnImGuiRender();
		
		if (s_ShowDemoWindow)
			ImGui::ShowDemoWindow(&s_ShowDemoWindow);

		ImGui::End();
	}

	void EditorLayer::UI_DrawTitleBar(float& outTitlebarHeight)
	{
		const float titlebarHeight = 58.0f;
		const bool isMaximed = Application::Get().GetWindow().IsMaximized();
		const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

		ImGui::SetCursorPos(windowPadding);

		const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
		const ImVec2 titlebarMax = titlebarMin + ImVec2(ImGui::GetWindowWidth() - windowPadding.y * 2.0f, titlebarHeight);

		ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
		ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();

		bgDrawList->AddRectFilled(titlebarMin, titlebarMax, UI::Colors::Theme::Titlebar);

		// TODO(moro): logo
		{
			const float logoSize = 48.0f;
			const ImVec2 logoOffset(16.0f, 5.0f);
			const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
			const ImVec2 logoRectMax = { logoRectStart.x + logoSize, logoRectStart.y + logoSize };
			fgDrawList->AddRectFilled(logoRectStart, logoRectMax, IM_COL32(240, 20, 20, 255));
		}

		ImGui::SetCursorPos(ImGui::GetCursorStartPos()); // Reset cursor pos
		ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.x * 2.0f, ImGui::GetFrameHeightWithSpacing() });

		const float w = ImGui::GetContentRegionAvail().x;
		const float buttonsAreaWidth = 94;

		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("##titlebarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));

		m_TitlebarHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped);

		ImGui::SuspendLayout();
		{
			ImGui::SetNextItemAllowOverlap();
			const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
			ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 0.0f));
			UI_DrawMenuBar();

			if (ImGui::IsItemHovered())
				m_TitlebarHovered = false;
		}
		ImGui::ResumeLayout();



		const float buttonWidth = 14.0f;
		const float buttonHeight = 14.0f;
		const ImU32 buttonColN = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 0.9f);
		const ImU32 buttonColH = UI::Colors::WithMultipliedValue(UI::Colors::Theme::Text, 1.2f);
		const ImU32 buttonColP = UI::Colors::Theme::TextDarker;

		ImGui::Spring();
		{
			const int iconWidth = EditorResources::WindowMinimizeIcon->GetWidth();
			const int iconHeight = EditorResources::WindowMinimizeIcon->GetHeight();
			const float padY = (buttonHeight - (float)iconHeight) / 2.0f;

			if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
			{
				Application::Get().SubmitToMainThread([]()
				{
					Window& window = Application::Get().GetWindow();
					window.Minimize();
				});
			}

			UI::DrawImageButton(EditorResources::WindowMinimizeIcon, buttonColN, buttonColH, buttonColP, UI::RectExpand(UI::GetItemRect(), 0.0f, -padY));
		}

		ImGui::Spring(-1.0f, 17.0f);
		{
			if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
			{
				Application::Get().SubmitToMainThread([isMaximed]()
				{
					Window& window = Application::Get().GetWindow();
					if (isMaximed)
						window.Restore();
					else
						window.Maximize();
				});
			}

			UI::DrawImageButton(isMaximed ? EditorResources::WindowRestoreIcon : EditorResources::WindowMaximizeIcon, buttonColN, buttonColH, buttonColP);
		}

		ImGui::Spring(-1.0f, 15.0f);
		{
			if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
			{
				Application::Get().SubmitToMainThread([]()
				{
					Window& window = Application::Get().GetWindow();
					window.KillWindow();
				});
			}

			UI::DrawImageButton(EditorResources::WindowCloseIcon, buttonColN, buttonColH, buttonColP);
		}

		ImGui::Spring(-1.0f, 18.0f);
		ImGui::EndHorizontal();


		ImGui::SetCursorPos(ImVec2(windowPadding.x, 0));
		ImGui::BeginHorizontal("##centerWindowTitle", { ImGui::GetWindowWidth() - windowPadding.x * 2.0f, ImGui::GetFrameHeightWithSpacing() });
		{

			ImGui::Spring(0.5f);

			UI::ScopedFont titleFont("Bold");
			ImGui::AlignTextToFramePadding();

			const std::string& windowTitle = Application::Get().GetWindow().GetTitle();
			const ImVec2 textSize = ImGui::CalcTextSize(windowTitle.data(), windowTitle.data() + windowTitle.length());
			const ImVec2 itemMin = ImGui::GetCursorScreenPos() + ImVec2(0.0f, ImGui::GetCurrentWindowRead()->DC.CurrLineTextBaseOffset);
			const ImVec2 itemMax = itemMin + textSize;

			const auto& style = ImGui::GetStyle();
			ImRect titleRect = UI::RectExpand({ itemMin, itemMax }, style.FramePadding * 2.0f);
			titleRect.Min.y = 0.0f;

			ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
			drawList->PushClipRectFullScreen();
			drawList->AddRectFilled(titleRect.Min, titleRect.Max, IM_COL32(135, 100, 255, 30), 5.0f, ImDrawFlags_RoundCornersBottom);
			drawList->PopClipRect();

			ImGui::Text(windowTitle.c_str());

			ImGui::Spring(0.5f);
		}
		ImGui::EndHorizontal();


		outTitlebarHeight = titlebarHeight;
	}

	void EditorLayer::UI_DrawMenuBar()
	{
		SK_PROFILE_FUNCTION();

		if (Application::Get().GetSpecification().CustomTitlebar)
		{
			UI::ScopedColor menubarColor(ImGuiCol_MenuBarBg, 0);
			const ImRect menubarRect = { ImGui::GetCursorPos(), { ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing() } };

			ImGui::BeginGroup();
			if (UI::BeginMenubar(menubarRect))
			{
				UI_DrawMenuBarItems();
				UI::EndMenubar();
			}
			ImGui::EndGroup();
		}
		else
		{
			if (ImGui::BeginMenuBar())
			{
				UI_DrawMenuBarItems();
				ImGui::EndMenuBar();
			}
		}
	}

	void EditorLayer::UI_DrawMenuBarItems()
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
				m_CreateProjectModal = { .Open = true };
			}

			if (ImGui::MenuItem("Open Project"))
				OpenProject();

			if (ImGui::BeginMenu("Recent Projects"))
			{
				auto& settings = EditorSettings::Get();
				for (const auto& [lastOpened, recentProject] : std::views::reverse(settings.RecentProjects))
				{
					if (ImGui::MenuItem(recentProject.Name.c_str()))
					{
						OpenProject(recentProject.Filepath);
						break;
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Save Project"))
			{
				ProjectSerializer serializer(Project::GetActive());
				serializer.Serialize(Project::GetProjectFilePath());
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit"))
				Application::Get().CloseApplication();

			ImGui::EndMenu();
		}

		m_PanelManager->DrawMenus();

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Theme Editor", nullptr, &m_ShowThemeEditor);

			if (ImGui::MenuItem("Reload Icons"))
				m_ReloadEditorIcons = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
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

			ImGui::MenuItem("Key States", nullptr, &m_ShowKeyStates);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Script"))
		{
			if (ImGui::MenuItem("Reload", nullptr, false, m_SceneState == SceneState::Edit))
			{
				//ScriptEngine::ScheduleReload();
				// #TODO(moro): should this happen here
				Project::RestartScriptEngine();
			}

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

			if (ImGui::MenuItem("Rebuild all mesh entity hierarchies"))
			{
				auto entities = m_EditorScene->GetAllEntitysWith<MeshComponent>();
				for (auto ent : entities)
				{
					Entity entity{ ent, m_EditorScene };
					m_EditorScene->RebuildMeshEntityHierarchy(entity);
				}
			}

			if (ImGui::MenuItem("Load all assets"))
			{
				AssetRegistry& reg = Project::GetEditorAssetManager()->GetAssetRegistry();

				AssetHandle lastHandle;
				for (const auto& [handle, metadata] : reg)
				{
					if (metadata.Status != AssetStatus::Unloaded)
						continue;

					AssetManager::GetAssetAsync(handle);
					lastHandle = handle;
				}

				auto future = AssetManager::GetAssetFuture(lastHandle);
				future.OnReady([](...)
				{
					SK_CONSOLE_INFO("Finished loading all Assets");
				});
			}

			if (ImGui::BeginMenu("Invalid Asset Handles"))
			{
				static const char* s_InvalidSceneMetadataFilepath = "{5CD97036-DB54-4709-9649-5263680CB27D}InvalidScene.skscene";

				ImGui::Text("Scene");
				if (ImGui::Button("Add Metadata"))
				{
					auto assetManager = Project::GetEditorAssetManager();
					AssetRegistry& registry = assetManager->GetAssetRegistry();

					AssetHandle handle = registry.TryFind(s_InvalidSceneMetadataFilepath);
					if (!handle)
					{
						handle = 1;
						while (registry.Contains(handle))
							handle = handle + 1;

						SK_CORE_VERIFY(registry.Contains(handle) ? (registry.Read(handle).FilePath == s_InvalidSceneMetadataFilepath) : true);

						AssetMetaData invalidMetadata;
						invalidMetadata.Handle = handle;
						invalidMetadata.Type = AssetType::Scene;
						invalidMetadata.FilePath = s_InvalidSceneMetadataFilepath;
						registry.Add(invalidMetadata);
						SK_CONSOLE_DEBUG("Created invalid metadata.\nType: Scene\nHandle: {}\nFilepath: {}", handle, s_InvalidSceneMetadataFilepath);
					}
				}

				if (ImGui::Button("Remove Metadata"))
				{
					auto assetManager = Project::GetEditorAssetManager();
					AssetRegistry& registry = assetManager->GetAssetRegistry();

					AssetHandle handle = registry.TryFind(s_InvalidSceneMetadataFilepath);
					if (handle)
					{
						registry.Remove(handle);
						SK_CONSOLE_DEBUG("Removed invalid metadata.\nType: Scene\nHandle: {}\nFilepath: {}", handle, s_InvalidSceneMetadataFilepath);
					}
				}

				ImGui::Button("Asset");
				if (ImGui::BeginDragDropSource())
				{
					auto assetManager = Project::GetEditorAssetManager();
					AssetRegistry& registry = assetManager->GetAssetRegistry();

					AssetHandle handle = registry.TryFind(s_InvalidSceneMetadataFilepath);
					if (!handle)
						handle = 1;

					ImGui::SetDragDropPayload("Asset", &handle, sizeof(AssetHandle));
					ImGui::EndDragDropSource();

				}

				ImGui::Separator();

				ImGui::Button("Scene Memory");
				if (ImGui::BeginDragDropSource())
				{
					static AssetHandle memoryAssetHandle;
					if (!memoryAssetHandle)
						memoryAssetHandle = AssetManager::CreateMemoryOnlyAsset<Scene>();

					ImGui::SetDragDropPayload("Asset", &memoryAssetHandle, sizeof(AssetHandle));
					ImGui::EndDragDropSource();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

	}

	void EditorLayer::UI_Viewport()
	{
		SK_PROFILE_FUNCTION();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("MainViewport", nullptr, ImGuiWindowFlags_None);
		ImGui::PopStyleVar(4);

		if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
			ImGui::SetWindowFocus();

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

		Ref<Image2D> fbImage = m_SceneRenderer->GetFinalPassImage();
		UI::Image(fbImage, size);

		UI_Gizmo();
		UI_DragDrop();

		ImGui::End();
	}

	void EditorLayer::UI_Gizmo()
	{
		SK_PROFILE_FUNCTION();
		
		if (!m_RenderGizmo)
			return;

		if (m_CurrentOperation != (ImGuizmo::OPERATION)0 && SelectionManager::AnySelected(m_EditorScene->GetID()))
		{
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
				auto& component = cameraEntity.GetComponent<CameraComponent>();
				glm::mat4 transform = cameraEntity.Transform().CalcTransform();

				ImGuizmo::SetOrthographic(!component.IsPerspective);
				view = glm::inverse(transform);
				projection = component.GetProjection();
			}
			else
			{
				ImGuizmo::SetOrthographic(false);
				view = m_EditorCamera.GetView();
				projection = m_EditorCamera.GetProjection();
			}

			auto entities = SelectionManager::GetSelections(m_EditorScene->GetID()) |
				            std::views::transform([s = m_EditorScene](UUID id) { return s->TryGetEntityByUUID(id); });

			TransformComponent centerTransform = m_ActiveScene->GetWorldSpaceTransform(entities[0]);
			centerTransform.Translation = utils::GetCenterOfEntities(m_ActiveScene, entities);
			glm::mat4 transform = centerTransform.CalcTransform();

			float snapVal = 0.0f;
			if (Input::IsKeyDown(KeyCode::LeftShift))
			{
				switch (m_CurrentOperation)
				{
					case ImGuizmo::TRANSLATE: snapVal = m_TranslationSnap; break;
					case ImGuizmo::ROTATE:    snapVal = m_RotationSnap; break;
					case ImGuizmo::SCALE:     snapVal = m_ScaleSnap; break;
				}
			}

			float snap[3] = { snapVal, snapVal, snapVal };
			glm::mat4 delta;
			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), (ImGuizmo::OPERATION)m_CurrentOperation, ImGuizmo::LOCAL, glm::value_ptr(transform), glm::value_ptr(delta), snap);

			if (!Input::IsKeyDown(KeyCode::LeftAlt) && ImGuizmo::IsUsing())
			{
				for (Entity entity : entities)
				{
					glm::mat4 originalTransform = m_ActiveScene->GetWorldSpaceTransformMatrix(entity);

					glm::vec3 translation, rotation, scale;
					if (Math::DecomposeTransform(delta, translation, rotation, scale))
					{
						auto& tf = entity.Transform();
						switch (m_CurrentOperation)
						{
							case ImGuizmo::TRANSLATE: tf.Translation += translation; break;
							case ImGuizmo::ROTATE: tf.Rotation += rotation; break;
							case ImGuizmo::SCALE: tf.Scale *= scale; break;
						}
					}
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
				UI::BeginControlsGrid();

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

				ImGui::TableNextRow();
				for (int i = 0; i < ImGui::TableGetColumnCount(); i++)
				{
					ImGui::TableSetColumnIndex(i);
					ImGui::Separator();
				}
				ImGui::TableNextRow();

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

				UI::EndControlsGrid();

				if (ImGui::Button("Reset"))
				{
					m_EditorCamera.SetFocusPoint({ 0.0f, 0.0f, 0.0f });
					m_EditorCamera.SetPicht(0.0f);
					m_EditorCamera.SetYaw(0.0f);
					m_EditorCamera.SetDistance(10.0f);
				}

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
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset");
			if (payload)
			{
				AssetHandle handle = *(AssetHandle*)payload->Data;
				if (AssetManager::IsValidAssetHandle(handle))
				{
					const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
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
							SelectionManager::DeselectAll(m_EditorScene->GetID());
							SelectionManager::Select(m_EditorScene->GetID(), entity.GetUUID());
							break;
						}
						case AssetType::Mesh:
						{
							AssetManager::GetAssetFuture(handle).OnReady([this](Ref<Asset> asset)
							{
								Entity entity = m_EditorScene->InstantiateMesh(asset.As<Mesh>());
								SelectionManager::DeselectAll(m_EditorScene->GetID());
								SelectionManager::Select(m_EditorScene->GetID(), entity.GetUUID());
							});
							break;
						}
						case AssetType::MeshSource:
						{
							m_CreateMeshAssetData = {};
							m_CreateMeshAssetData.Show = true;
							m_CreateMeshAssetData.MeshSource = handle;
							m_CreateMeshAssetData.DestinationPath = FileSystem::CreatePathString({}, FileSystem::GetStemString(metadata.FilePath), ".skmesh");
							m_CreateMeshAssetData.ParentDirectory = "Assets/Meshes/";
							break;
						}
						case AssetType::Prefab:
						{
							AssetManager::GetAssetFuture(handle).OnReady([this](Ref<Asset> asset)
							{
								Entity entity = m_EditorScene->Instansitate(asset.As<Prefab>());
								SelectionManager::DeselectAll(m_EditorScene->GetID());
								SelectionManager::Select(m_EditorScene->GetID(), entity.GetUUID());
							});
						}
					}
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

		const float windowContentRegionWith = ImGui::GetContentRegionAvail().x;
		ImGui::SetCursorPosX(windowContentRegionWith * 0.5f - (size.x * 3.0f * 0.5f) - (style.ItemSpacing.x * 3.0f));

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				if (UI::ImageButton("Play", EditorResources::PlayIcon, size))
					OnScenePlay();

				ImGui::SameLine();

				if (UI::ImageButton("Simulate", EditorResources::SimulateIcon, size))
					OnSimulationPlay();

				ImGui::SameLine();
				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, true);
					UI::ImageButton("Step Disabled", EditorResources::StepIcon, size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, { 0.5f, 0.5f, 0.5f, 1.0f });
				}

				break;
			}
			case SceneState::Play:
			{
				if (UI::ImageButton("Stop", EditorResources::StopIcon, size))
					OnSceneStop();

				ImGui::SameLine();

				Ref<Texture2D> pausePlayIcon = m_ActiveScene->IsPaused() ? EditorResources::PlayIcon : EditorResources::PauseIcon;
				if (UI::ImageButton("PausePlay", pausePlayIcon, size))
					m_ActiveScene->SetPaused(!m_ActiveScene->IsPaused());

				ImGui::SameLine();

				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, !m_ActiveScene->IsPaused());
					const ImVec4 tintColor = m_ActiveScene->IsPaused() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
					if (UI::ImageButton("Step", EditorResources::StepIcon, size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, tintColor))
						m_ActiveScene->Step(1);
				}

				break;
			}
			case SceneState::Simulate:
			{
				if (UI::ImageButton("StopIcon", EditorResources::StopIcon, size))
					OnSimulationStop();

				ImGui::SameLine();

				Ref<Texture2D> pausePlayIcon = m_ActiveScene->IsPaused() ? EditorResources::PlayIcon : EditorResources::PauseIcon;
				if (UI::ImageButton("PausePlayIcon", pausePlayIcon, size))
					m_ActiveScene->SetPaused(!m_ActiveScene->IsPaused());

				ImGui::SameLine();

				{
					UI::ScopedItemFlag disabled(ImGuiItemFlags_Disabled, !m_ActiveScene->IsPaused());
					const ImVec4 tintColor = m_ActiveScene->IsPaused() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
					if (UI::ImageButton("Step", EditorResources::StepIcon, size, {0, 0}, {1, 1}, {0, 0, 0, 0}, tintColor))
						m_ActiveScene->Step(1);
				}

				break;
			}
		}

		ImGui::End();
	}

	bool EditorLayer::UI_MousePicking()
	{
		// #Renderer #Disabled mouse picking is not yet supported
		return false;
#if TODO
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

			if (selectEntity)
			{
				Renderer::CopyImage(m_SceneRenderer->GetIDImage(), m_MousePickingImage);

				Renderer::Submit([this, x, y]()
				{
					uint32_t hoverdEntity = (uint32_t)-1;
					// #Renderer #Disabled Image2D::RT_ReadPixel is not implemented and this is not on the render thread
					if (!m_MousePickingImage->RT_ReadPixel(x, y, hoverdEntity))
						return;

					if (hoverdEntity == (uint32_t)-1)
					{
						SelectionManager::DeselectAll(m_EditorScene->GetID());
						return;
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

						if (Input::IsKeyDown(KeyCode::LeftControl))
						{
							UUID entityID = entity.GetUUID();
							const bool isSelected = SelectionManager::IsSelected(m_EditorScene->GetID(), entityID);
							SelectionManager::Toggle(m_EditorScene->GetID(), entityID, !isSelected);
						}
						else
						{
							SelectionManager::DeselectAll(m_EditorScene->GetID());
							SelectionManager::Select(m_EditorScene->GetID(), entity.GetUUID());
						}
					}
				});
			}
		}
		return true;
#endif
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
			ImGui::Text(m_OpenProjectModal.ProjectFile);

			const ImGuiStyle& style = ImGui::GetStyle();

			ImVec2 openTextSize = ImGui::CalcTextSize("Open");
			ImVec2 closeTextSize = ImGui::CalcTextSize("Close");
			
			ImVec2 openButtonSize = { openTextSize.x + style.FramePadding.x * 2.0f, openTextSize.y + style.FramePadding.y * 2.0f };
			ImVec2 closeButtonSize = { closeTextSize.x + style.FramePadding.x * 2.0f, closeTextSize.y + style.FramePadding.y * 2.0f };

			ImVec2 contentRegion = ImGui::GetContentRegionAvail() + ImGui::GetCursorScreenPos() - ImGui::GetWindowPos();

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

			ImGui::Text(fmt::format("Importing Asset from {}", m_ImportAssetData.SourcePath));

			char buffer[MAX_PATH];
			strcpy_s(buffer, m_ImportAssetData.DestinationPath.c_str());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("...").x + style.FramePadding.x * 2.0f - style.ItemSpacing.x);

			bool invalidInput;
			if (UI::InputText("##destPath", buffer, MAX_PATH, ImGuiInputTextFlags_CallbackCharFilter, UI_INPUT_TEXT_FILTER(":*?\"<>|")))
				m_ImportAssetData.DestinationPath = buffer;

#if TODO
			if (invalidInput)
			{
				m_ImportAssetData.Error = "File paths aren't allowed to containe any of the folloing character!\n:*?\"<>|";
				m_ImportAssetData.ShowError = true;
			}
#endif

			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				auto path = Platform::SaveFileDialog(L"", 1, Project::GetActiveAssetsDirectory(), true, false);
				path.replace_extension();
				m_ImportAssetData.DestinationPath = std::filesystem::relative(path, Project::GetActiveAssetsDirectory()).generic_string();
			}

			if (m_ImportAssetData.ShowError)
			{
				UI::ScopedColor errorText(ImGuiCol_Text, UI::Colors::Theme::TextError);
				ImGui::Text(m_ImportAssetData.Error);
				m_ImportAssetData.ShowErrorTimer += m_TimeStep;
				if (m_ImportAssetData.ShowErrorTimer >= m_ImportAssetData.ShowErrorDuration)
				{
					m_ImportAssetData.ShowError = false;
					m_ImportAssetData.ShowErrorTimer = 0.0f;
				}
			}

			ImVec2 importTextSize = ImGui::CalcTextSize("Import");
			ImVec2 cancleTextSize = ImGui::CalcTextSize("Cancel");

			ImVec2 importButtonSize = { importTextSize.x + style.FramePadding.x * 2.0f, importTextSize.y + style.FramePadding.y * 2.0f };
			ImVec2 cancleButtonSize = { cancleTextSize.x + style.FramePadding.x * 2.0f, cancleTextSize.y + style.FramePadding.y * 2.0f };

			ImVec2 contentRegion = ImGui::GetContentRegionAvail() + ImGui::GetCursorScreenPos() - ImGui::GetWindowPos();

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
						Project::GetEditorAssetManager()->ImportAsset(destination);
						m_ImportAssetData = {};
					}
				}
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
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
			ImGui::TextWrapped("This file cannot be used directly, a Shark (.skmesh) file must first be created to interpret the contents of the file.");
			ImGui::Separator();

			UI::BeginControlsGrid();
			std::string label = fmt::format("path: {}", m_CreateMeshAssetData.ParentDirectory);
			UI::Control(label, m_CreateMeshAssetData.DestinationPath);
			UI::EndControlsGrid();

			if (ImGui::Button("Create"))
			{
				std::filesystem::path fullPath = m_CreateMeshAssetData.ParentDirectory / m_CreateMeshAssetData.DestinationPath;
				FileSystem::ReplaceExtension(fullPath, ".skmesh");
				
				AssetHandle meshSource = m_CreateMeshAssetData.MeshSource;
				if (AssetManager::IsValidAssetHandle(meshSource) && AssetManager::GetAssetType(meshSource) == AssetType::MeshSource)
				{
					Ref<Mesh> mesh = Project::GetEditorAssetManager()->CreateAsset<Mesh>(fullPath, meshSource);
					Entity entity = m_EditorScene->InstantiateMesh(mesh);
					SelectionManager::DeselectAll(m_EditorScene->GetID());
					SelectionManager::Select(m_EditorScene->GetID(), entity.GetUUID());
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_CreateMeshAssetData = {};
			}

		}
		ImGui::End();

	}

	void EditorLayer::UI_CreateProjectModal()
	{
		if (m_CreateProjectModal.Open)
		{
			ImGui::OpenPopup("New Project");
			m_CreateProjectModal.Open = false;
			m_CreateProjectModal.Show = true;
		}

		if (!m_CreateProjectModal.Show)
			return;

		if (ImGui::BeginPopupModal("New Project", &m_CreateProjectModal.Show))
		{
			std::string fullPath = fmt::format("{}/{}.skproj", m_CreateProjectModal.Location, m_CreateProjectModal.Name);
			ImGui::Text("Full path: %s", fullPath.c_str());

			ImGui::SetNextItemWidth(-1.0f);
			UI::InputTextWithHint("##Name", "Project Name", &m_CreateProjectModal.Name);

			const ImGuiStyle& style = ImGui::GetStyle();
			ImVec2 offsetSize = ImGui::CalcTextSize("...") + style.FramePadding * 2.0f;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - offsetSize.x - ImGui::GetStyle().ItemSpacing.x);
			UI::InputTextWithHint("##Location", "Project Location", &m_CreateProjectModal.Location);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				std::filesystem::path directory = Platform::OpenDirectoryDialog();
				if (!directory.empty())
					m_CreateProjectModal.Location = directory.generic_string();
			}

			if (ImGui::Button("Create"))
			{
				SK_CORE_VERIFY(false, "Not Implemented");
				// TODO(moro): Create Project
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				m_CreateProjectModal.Show = false;
		}
		ImGui::End();
	}

	void EditorLayer::UI_ShowKeyStates()
	{
		if (!m_ShowKeyStates)
			return;

		if (ImGui::Begin("Key States", &m_ShowKeyStates))
		{
			constexpr auto name = magic_enum::enum_name(KeyCode::OemComma);
			constexpr auto name2 = magic_enum::enum_name(KeyCode::A);

#if TODO
			const auto& keyStates = Input::GetKeyStates();
			for (const auto& [key, state] : keyStates)
			{
				UI::ScopedColorConditional textColor(ImGuiCol_Text, IM_COL32(20, 225, 30, 255), state == KeyState::Down);
				ImGui::Text(fmt::format("{}: {}", key, state));
			}
#endif
		}
		ImGui::End();
	}

	void EditorLayer::UpdateMainWindow()
	{
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
			for (UUID entityID : SelectionManager::GetSelections(m_EditorScene->GetID()))
			{
				Entity entity = m_ActiveScene->TryGetEntityByUUID(entityID);
				if (entity.HasComponent<PointLightComponent>())
				{
					const auto& plc = entity.GetComponent<PointLightComponent>();

					glm::vec3 center = entity.Transform().Translation;
					glm::vec4 lightRadiusColor = { 0.1f, 0.3f, 0.9f, 1.0f };
					float rotation = glm::radians(90.0f);

					m_DebugRenderer->DrawCircle(center, { 0.0f, 0.0f, 0.0f }, plc.Radius, lightRadiusColor);
					m_DebugRenderer->DrawCircle(center, { 0.0f, rotation, 0.0f }, plc.Radius, lightRadiusColor);
					m_DebugRenderer->DrawCircle(center, { rotation, 0.0f, 0.0f }, plc.Radius, lightRadiusColor);
				}
			}
		}

		m_DebugRenderer->EndScene();
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

		AssetHandle handle = AssetManager::CreateMemoryOnlyAsset<Scene>(name);
		LoadScene(handle);
	}

	bool EditorLayer::LoadScene(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		Ref<Scene> scene = AssetManager::GetAsset<Scene>(handle);
		if (!scene)
			return false;

		if (m_RuntimeScene)
			OnSceneStop();
		if (m_SimulateScene)
			OnSimulationStop();

		if (m_ActiveScene && AssetManager::IsMemoryAsset(m_ActiveScene->Handle))
			AssetManager::DeleteMemoryAsset(m_ActiveScene->Handle);

		SelectionManager::DeselectAll();

		m_EditorScene = scene;
		m_EditorScene->IsEditorScene(true);
		m_EditorScene->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		m_PanelManager->SetContext(m_EditorScene);
		ScriptEngine::Get().SetCurrentScene(m_EditorScene);
		m_ActiveScene = m_EditorScene;

		m_EditorCamera.SetFlyView({ 40.0f, 25.0f, -40.0f }, 10.0f, -45.0f);
		Application::Get().SubmitToMainThread([this]() { UpdateWindowTitle(); });
	}

	bool EditorLayer::LoadScene(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		AssetHandle assetHandle = Project::GetEditorAssetManager()->GetAssetHandleFromFilepath(filePath);
		if (!AssetManager::IsValidAssetHandle(assetHandle))
			return false;

		return LoadScene(assetHandle);
	}

	bool EditorLayer::SaveScene()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_EditorScene);

		if (AssetManager::IsMemoryAsset(m_EditorScene->Handle))
			return SaveSceneAs();

		return Project::GetEditorAssetManager()->SaveAsset(m_EditorScene->Handle);
	}

	bool EditorLayer::SaveSceneAs()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);
		SK_CORE_ASSERT(m_ActiveScene == m_EditorScene);

		auto filePath = Platform::SaveFileDialog(L"|*.*|Scene|*.skscene", 2, Project::GetActiveAssetsDirectory(), true);
		if (filePath.empty())
			return false;

		SK_CORE_ASSERT(filePath.extension() == L".skscene");
		if (filePath.extension() != L".skscene")
			return false;

		if (AssetManager::IsMemoryAsset(m_EditorScene->Handle))
		{
			std::string directoryPath = Project::GetEditorAssetManager()->MakeRelativePathString(filePath.parent_path());
			std::string fileName = filePath.filename().string();
			return Project::GetEditorAssetManager()->ImportMemoryAsset(m_EditorScene->Handle, directoryPath, fileName);
		}

		std::string directoryPath = Project::GetEditorAssetManager()->MakeRelativePathString(filePath.parent_path());
		std::string fileName = filePath.filename().string();
		Ref<Scene> newScene = Project::GetEditorAssetManager()->CreateAsset<Scene>(directoryPath, fileName);
		m_EditorScene->CopyTo(newScene);
		return Project::GetEditorAssetManager()->SaveAsset(newScene->Handle);
	}

	void EditorLayer::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Play;
		m_RenderGizmo = false;

		m_RuntimeScene = Scene::Copy(m_EditorScene);
		OnSceneStateChanged(m_RuntimeScene);

		m_PanelManager->OnScenePlay();
		m_RuntimeScene->OnScenePlay();
	}

	void EditorLayer::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_RenderGizmo = true;

		m_RuntimeScene->OnSceneStop();
		m_PanelManager->OnSceneStop();

		OnSceneStateChanged(m_EditorScene);
		m_RuntimeScene = nullptr;

		Project::RestartScriptEngine();
	}

	void EditorLayer::OnSimulationPlay()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(m_SceneState == SceneState::Edit);

		m_SceneState = SceneState::Simulate;
		m_RenderGizmo = false;

		m_SimulateScene = Scene::Copy(m_EditorScene);
		OnSceneStateChanged(m_SimulateScene);

		m_ActiveScene->OnSimulationPlay();
	}

	void EditorLayer::OnSimulationStop()
	{
		SK_PROFILE_FUNCTION();

		m_SceneState = SceneState::Edit;
		m_RenderGizmo = true;

		OnSceneStateChanged(m_EditorScene);
		m_SimulateScene = nullptr;

		m_ActiveScene->OnSimulationStop();
	}

	void EditorLayer::OnSceneStateChanged(Ref<Scene> stateScene)
	{
		ScriptEngine::Get().SetCurrentScene(stateScene);
		m_PanelManager->SetContext(stateScene);
		m_ActiveScene = stateScene;
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

		Ref<ProjectConfig> config = Ref<ProjectConfig>::Create();
		ProjectSerializer serializer(config);
		if (!serializer.Deserialize(filePath))
		{
			SK_CORE_ERROR_TAG("Core", "Failed to open project [{}]", filePath);
			SK_CONSOLE_ERROR("Failed to load project file.\n{}", filePath);
			return;
		}

		if (Project::GetActive())
			CloseProject();

		Project::SetActive(config);

		if (FileSystem::Exists(Project::GetActive()->GetScriptModulePath()))
		{
			ScriptEngine::Get().LoadAppAssembly();
			m_ScriptEngineLastModifiedTime = FileSystem::GetLastWriteTime(Project::GetActive()->GetScriptModulePath());
		}

		auto& settings = EditorSettings::Get();
		std::erase_if(settings.RecentProjects, [filePath](const auto& node) { return node.second.Filepath == filePath; });

		RecentProject recentProject;
		recentProject.Name = config->Name;
		recentProject.Filepath = config->GetProjectFilepath();
		recentProject.LastOpened = std::chrono::system_clock::now();
		settings.RecentProjects[recentProject.LastOpened] = std::move(recentProject);

		m_PanelManager->OnProjectChanged(config);

		if (!LoadScene(config->StartupScene))
			NewScene("Empty Fallback Scene");
		//LoadScene(project->GetConfig().StartupScene);
	}

	void EditorLayer::CloseProject()
	{
		SK_PROFILE_FUNCTION();

		if (m_RuntimeScene)
			OnSceneStop();
		if (m_SimulateScene)
			OnSimulationStop();

		{
			ProjectSerializer serializer(Project::GetActive());
			serializer.Serialize(Project::GetProjectFilePath());
		}

		Project::GetEditorAssetManager()->SerializeImportedAssets();

		SK_CORE_INFO_TAG("Core", "Closing Project");

		SelectionManager::DeselectAll();

		m_ActiveScene = nullptr;
		m_PanelManager->SetContext(nullptr);
		ScriptEngine::Get().SetCurrentScene(nullptr);
		m_EditorScene = nullptr;

		m_PanelManager->OnProjectChanged(nullptr);

		Project::GetEditorAssetManager()->PrepareForQuickStop();
		Project::SetActive(nullptr);

		Application::Get().SubmitToMainThread([this]() { UpdateWindowTitle(); });
	}
	
	Ref<ProjectConfig> EditorLayer::CreateProject(const std::filesystem::path& projectDirectory)
	{
		// Create Directory
		SK_NOT_IMPLEMENTED();
		return nullptr;
#if TODO
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

		CreateProjectPremakeFile(project);
		std::filesystem::copy_file("Resources/Project/Setup.bat", fmt::format("{0}/Setup.bat", project->GetDirectory()));
		std::filesystem::copy("Resources/Project/Premake", project->GetDirectory() / "Premake", std::filesystem::copy_options::recursive);

		ProjectSerializer serializer(project);
		serializer.Serialize(fmt::format("{0}/{1}.skproj", project->GetDirectory(), project->GetConfig().Name));

		return project;
#endif
	}

	void EditorLayer::CreateProjectPremakeFile(Ref<ProjectConfig> project)
	{
		const std::string projectNameToken = "%PROJECT_NAME%";

		std::string premakeTemplate = FileSystem::ReadString("Resources/Project/PremakeFileTemplate.lua");
		String::Replace(premakeTemplate, projectNameToken,  project->Name);

		auto premakeFilePath = fmt::format("{0}/premake5.lua", project->GetDirectory());
		FileSystem::WriteString(premakeFilePath, premakeTemplate);
	}

	void EditorLayer::RunScriptSetup()
	{
		ExecuteSpecs specs;
		specs.Target = fmt::format(L"{}/Premake/premake5.exe", Project::GetActiveDirectory());
		// vs2022 doesn't work for some reason but vs2019 still generates vs2022 solution
		auto sharkDir = std::filesystem::current_path().parent_path();
		specs.Params = L"vs2019";
		specs.WaitUntilFinished = true;
		specs.WorkingDirectory = Project::GetActiveDirectory();
		specs.InterhitConsole = true;
		Platform::Execute(specs);
	}

	void EditorLayer::OpenIDE()
	{
		auto solutionPath = fmt::format("{}/{}.sln", Project::GetActiveDirectory(), Project::GetName());
		Platform::Execute(ExecuteVerb::Run, solutionPath);
	}

	void EditorLayer::UpdateWindowTitle()
	{
		SK_PROFILE_FUNCTION();
		// Scene File name (Scene Name) - Editor Name - Platform - Renderer

		std::string sceneFilePath;
		std::string sceneName;
		if (m_ActiveScene)
		{
			auto& metadata = Project::GetEditorAssetManager()->GetMetadata(m_ActiveScene);
			sceneFilePath = metadata.FilePath.filename().string();
			sceneName = m_ActiveScene->GetName();
		}

		std::string title = fmt::format("{} ({}) - Shark-Editor - {} {} ({}) - {}", sceneFilePath, sceneName, Platform::GetPlatformName(), Platform::GetArchitecture(), Platform::GetConfiguration(), RendererAPI::GetCurrentAPI());
		Application::Get().GetWindow().SetTitle(title);
	}

	void EditorLayer::ReloadScriptEngineIfNeeded()
	{
		auto scriptModulePath = Project::GetActive()->GetScriptModulePath();
		if (FileSystem::Exists(scriptModulePath))
		{
			uint64_t scriptEngineLastModified = FileSystem::GetLastWriteTime(scriptModulePath);
			if (scriptEngineLastModified != m_ScriptEngineLastModifiedTime)
			{
				Project::RestartScriptEngine();
				m_ScriptEngineLastModifiedTime = scriptEngineLastModified;
			}
		}
	}

}
