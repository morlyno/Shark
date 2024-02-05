#include "skfpch.h"
#include "MaterialEditorPanel.h"

#include "Shark/Render/Renderer.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	MaterialEditorPanel::MaterialEditorPanel(const std::string& panelName, ImGuiID parentDockspaceID, Ref<MaterialAsset> material)
		: EditorPanel(panelName, parentDockspaceID), m_Material(material)
	{
	}

	MaterialEditorPanel::~MaterialEditorPanel()
	{
		Project::GetActiveEditorAssetManager()->RemoveAsset(m_Sphere);
	}

	void MaterialEditorPanel::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (!m_Active || !m_IsInitialized)
			return;

		if (m_NeedsResize && m_ViewportSize.x != 0 && m_ViewportSize.y != 0)
		{
			SK_CORE_DEBUG("Resize {}", m_ViewportSize);
			m_Camera.Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_Scene->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_Renderer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_NeedsResize = false;
		}

		m_Camera.OnUpdate(ts, m_ViewportHovered/* || m_ViewportFocused*/);
		m_Scene->OnRenderEditor(m_Renderer, m_Camera);
	}

	void MaterialEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		if (!shown)
			return;

		if (m_IsFirstFrame)
		{
			Initialize();
			m_IsFirstFrame = false;
		}

		if (!m_IsInitialized)
			return;

		if (!m_Active)
		{
			shown = false;
			destroy = true;
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowDockID(m_ParentDockspaceID, ImGuiCond_Appearing);
		const bool opened = ImGui::Begin(m_PanelName.c_str(), &m_Active);
		ImGui::PopStyleVar(2);

		if (!opened)
		{
			ImGui::End();
			return;
		}

		ImGui::DockSpace(m_DockspaceID);
		ImGui::End();

		if (!m_Tiny)
			DrawViewport();
		DrawSettings();

	}

	void MaterialEditorPanel::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<KeyPressedEvent>([this](KeyPressedEvent& e)
		{
			if (e.GetKeyCode() == KeyCode::F)
			{
				m_Camera.SetFocusPoint(glm::vec3(0.0f));
				return true;
			}
			return false;
		});
	}

	void MaterialEditorPanel::DrawSettings()
	{
		ImGui::SetNextWindowDockID(m_SettingsDockID);
		ImGui::Begin(m_SettingsName.c_str());

		m_MaterialEditor->DrawInline();

		if (ImGui::BeginPopupContextWindow("PanelSettings", ImGuiPopupFlags_MouseButtonRight))
		{
			if (m_Tiny)
			{
				if (ImGui::Button("Make Big"))
					m_Tiny = false;
			}
			else
			{
				if (ImGui::Button("Make Tiny"))
					m_Tiny = true;
			}
			ImGui::EndPopup();
		}


		if (ImGui::Button("Save"))
		{
			Project::GetActiveEditorAssetManager()->SaveAsset(m_Material->Handle);
		}

		ImGui::End();
	}

	void MaterialEditorPanel::DrawViewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowDockID(m_ViewportDockID);
		ImGui::Begin(m_ViewportName.c_str());
		ImGui::PopStyleVar(3);

		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x <= 0 || size.y <= 0)
			size = { (float)m_ViewportSize.x, (float)m_ViewportSize.y };

		if ((float)m_ViewportSize.x != size.x || (float)m_ViewportSize.y != size.y)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			m_ViewportSize.x = ImGui::GetContentRegionAvail().x;
			m_ViewportSize.y = ImGui::GetContentRegionAvail().y;
			m_NeedsResize = true;
		}

		m_ViewportHovered = ImGui::IsWindowHovered();
		m_ViewportFocused = ImGui::IsWindowFocused();

		// Note(moro): The InvisibleButton prevents imgui form moving the window form anywere but the titlebar
		//             This is currently the only way to do so as ImGuiIO::ConfigWindowsMoveFromTitleBarOnly dosn't work per window
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::InvisibleButton("Dummy", size);
		ImGui::SetCursorPos(cursor);
		ImGui::Image(m_Renderer->GetFinalImage()->GetViewID(), size);

		ImGui::End();
	}

	void MaterialEditorPanel::Initialize()
	{
		if (m_IsInitialized)
			return;

		m_MaterialEditor = Scope<MaterialEditor>::Create("", m_Material);

		ImGuiWindow* window = ImGui::FindWindowByName(m_PanelName.c_str());
		if (window && window->LastFrameActive == (ImGui::GetFrameCount() - 1))
		{
			SK_CORE_ERROR_TAG("UI", "MaterialEditorPanel::Initialize got called twice");
			m_Active = false;
			return;
		}

		SetupWindows();

		Ref<MaterialEditorPanel> instance = this;
		Application::Get().SubmitToMainThread([instance]()
		{
			instance->SetupSceneAndRenderer();
			instance->m_IsInitialized = true;
		});

	}

	void MaterialEditorPanel::SetupSceneAndRenderer()
	{
		m_Camera.SetProjection(m_ViewportSize.y / m_ViewportSize.x, 45.0f, 1.0f, 100.0f);

		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);

		AssetHandle sphereSourceHandle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Meshes/Default/Sphere.gltf");
		Ref<MeshSource> sphereSource = AssetManager::GetAsset<MeshSource>(sphereSourceHandle);
		m_Sphere = AssetManager::CreateMemoryAsset<Mesh>(sphereSource);
		Ref<Mesh> sphere = AssetManager::GetAsset<Mesh>(m_Sphere);
		sphere->GetMaterialTable()->SetMaterial(0, m_Material);

		Entity entity = m_Scene->CreateEntity("Sphere");
		auto& mc = entity.AddComponent<MeshRendererComponent>();
		mc.MeshHandle = sphere->Handle;
		mc.SubmeshIndex = 0;

		Entity lightEntity = m_Scene->CreateEntity("Light");
		lightEntity.Transform().Translation = { -4.0f, 3.0f, -4.0f };
		auto& pl = lightEntity.AddComponent<PointLightComponent>();
		pl.Intensity = 50.0f;
	}

	void MaterialEditorPanel::SetupWindows()
	{
		m_ViewportName = fmt::format("Viewport##{}", m_Material->Handle);
		m_SettingsName = fmt::format("Settings##{}", m_Material->Handle);

		ImGuiID tempDockspaceID = (ImGuiID)m_Material->Handle;
		m_DockspaceID = ImGui::DockBuilderAddNode(tempDockspaceID, ImGuiDockNodeFlags_DockSpace);

		ImGuiDockNode* parentDockspaceNode = ImGui::DockBuilderGetNode(m_ParentDockspaceID);
		ImGui::DockBuilderSetNodeSize(m_DockspaceID, parentDockspaceNode->Size);
		ImGui::DockBuilderSplitNode(m_DockspaceID, ImGuiDir_Left, 0.65f, &m_ViewportDockID, &m_SettingsDockID);

		ImGuiDockNode* viewportNode = ImGui::DockBuilderGetNode(m_ViewportDockID);
		viewportNode->SetLocalFlags(ImGuiDockNodeFlags_AutoHideTabBar);

		ImGui::DockBuilderFinish(m_DockspaceID);

		viewportNode = ImGui::DockBuilderGetNode(m_ViewportDockID);
		m_ViewportSize = { viewportNode->Size.x, viewportNode->Size.y };
	}

	MaterialEditor::MaterialEditor(const std::string& name, Ref<MaterialAsset> material)
		: m_Name(name), m_Material(material)
	{
	}

	void MaterialEditor::Draw()
	{
		ImGui::Begin(m_Name.c_str());
		DrawInline();
		ImGui::End();
	}

	void MaterialEditor::DrawInline()
	{
		if (!m_Material)
			return;

		UI::ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly, m_Readonly);

		const ImVec2 textureSize = { 64, 64 };

		UI::TextF("Shader: {}", m_Material->GetMaterial()->GetShader()->GetName());
		UI::TextF("Name: {}", m_Material->GetMaterial()->GetName());

		if (ImGui::CollapsingHeader("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = m_Material->GetAlbedoMap();
			SK_CORE_ASSERT(displayTexture);
			if (!displayTexture || displayTexture == Renderer::GetWhiteTexture())
			{
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAsset<Texture2D>(handle);
				hasTexture = false;
			}

			if (UI::TextureEdit(displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					m_Material->SetAlbedoMap(displayTexture);
				else
					m_Material->ClearAlbedoMap();
			}

			ImGui::SameLine();
			ImGui::ColorEdit3("Color", glm::value_ptr(m_Material->GetAlbedoColor()), ImGuiColorEditFlags_NoInputs);
		}

		if (ImGui::CollapsingHeader("Normal", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = m_Material->GetNormalMap();
			SK_CORE_ASSERT(displayTexture);
			if (!displayTexture || displayTexture == Renderer::GetWhiteTexture())
			{
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAsset<Texture2D>(handle);
				hasTexture = false;
			}

			if (UI::TextureEdit(displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					m_Material->SetNormalMap(displayTexture);
				else
					m_Material->ClearNormalMap();
			}

			ImGui::SameLine();

			bool usingNormalMap = m_Material->IsUsingNormalMap();
			if (ImGui::Checkbox("Enable", &usingNormalMap))
				m_Material->SetUsingNormalMap(usingNormalMap);
		}

		if (ImGui::CollapsingHeader("Metalness", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = m_Material->GetMetalnessMap();
			SK_CORE_ASSERT(displayTexture);
			if (!displayTexture || displayTexture == Renderer::GetWhiteTexture())
			{
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAsset<Texture2D>(handle);
				hasTexture = false;
			}

			if (UI::TextureEdit(displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					m_Material->SetMetalnessMap(displayTexture);
				else
					m_Material->ClearMetalnessMap();
			}

			ImGui::SameLine();
			ImGui::SliderFloat("Metalness Value", &m_Material->GetMetalness(), 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Roughness", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool hasTexture = true;
			Ref<Texture2D> displayTexture = m_Material->GetRoughnessMap();
			SK_CORE_ASSERT(displayTexture);
			if (!displayTexture || displayTexture == Renderer::GetWhiteTexture())
			{
				AssetHandle handle = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Textures/NoImagePlaceholder.sktex");
				displayTexture = AssetManager::GetAsset<Texture2D>(handle);
				hasTexture = false;
			}

			if (UI::TextureEdit(displayTexture, textureSize, hasTexture))
			{
				if (displayTexture)
					m_Material->SetRoughnessMap(displayTexture);
				else
					m_Material->ClearRoughnessMap();
			}

			ImGui::SameLine();
			ImGui::SliderFloat("Roughness Value", &m_Material->GetRoughness(), 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("PBR", ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::BeginControlsGrid();
			UI::Control("AmbientOcclusion", m_Material->GetAmbientOcclusion(), 0.005f, 0.0f, 1.0f);
			UI::EndControls();
		}

	}

}
