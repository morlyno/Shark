#include "skfpch.h"
#include "MaterialEditorPanel.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshFactory.h"
#include "Shark/Asset/ResourceManager.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"

namespace Shark {

	MaterialEditorPanel::MaterialEditorPanel(const std::string& panelName, ImGuiID parentDockspaceID, Ref<MaterialAsset> material)
		: EditorPanel(panelName, parentDockspaceID), m_Material(material)
	{
	}

	void MaterialEditorPanel::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (!m_IsInitialized)
			return;

		if (!m_Active)
			return;

		if (m_NeedsResize && m_ViewportSize.x != 0 && m_ViewportSize.y != 0)
		{
			SK_CORE_DEBUG("Resize {}", m_ViewportSize);
			m_Camera.Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_Renderer->Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_NeedsResize = false;
		}

		if (m_ViewportHovered || m_ViewportFocused)
			m_Camera.OnUpdate(ts);

		m_Scene->OnRender(m_Renderer, m_Camera.GetViewProjection());
	}

	void MaterialEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		if (!shown)
			return;

		if (!m_Active)
		{
			shown = false;
			destroy = true;
			return;
		}

		if (m_IsFirstFrame)
		{
			Initialize();
			m_IsFirstFrame = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowDockID(m_ParentDockspaceID, ImGuiCond_Appearing);
		const bool opened = ImGui::Begin(m_PanelName.c_str(), &m_Active, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(2);

		if (!opened)
		{
			ImGui::End();
			return;
		}

		ImGui::DockSpace(m_DockspaceID);
		ImGui::End();

		DrawViewport();
		DrawSettings();

	}

	void MaterialEditorPanel::DrawSettings()
	{
		ImGui::SetNextWindowDockID(m_SettingsDockID);
		ImGui::Begin(m_SettingsName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);

		UI::TextF("Shader: {}", m_Material->GetMaterial()->GetShader()->GetName());

		if (ImGui::TreeNodeEx("Albedo", ImGuiTreeNodeFlags_DefaultOpen | UI::DefaultHeaderFlags))
		{
			UI::BeginControlsGrid();

			glm::vec3 color = m_Material->GetAlbedoColor();
			if (UI::ControlColor("Color", color))
				m_Material->SetAlbedoColor(color);

			AssetHandle textureHandle = m_Material->GetAlbedoTexture();
			if (UI::ControlAsset("Texture", textureHandle))
				m_Material->SetAlbedoTexture(textureHandle);

			bool use = m_Material->UseAlbedo();
			if (UI::Control("Enable", use))
				m_Material->SetUseAlbedo(use);

			UI::EndControls();
			ImGui::TreePop();
		}

		if (m_Material->IsDirty())
		{
			//Ref<Material> mat = m_Material->GetMaterial();
			//mat->SetFloat3("u_PBRData.Albedo", m_Material->GetAlbedoColor());
			//mat->SetTexture("u_Albedo", m_Material->UseAlbedo() ? ResourceManager::GetAsset<Texture2D>(m_Material->GetAlbedoTexture()) : Renderer::GetWhiteTexture());
			//m_Material->SetDirty(false);
			m_Material->UpdateMaterial();
		}

		if (ImGui::Button("Save"))
		{
			ResourceManager::SaveAsset(m_Material->Handle);
		}

		ImGui::End();
	}

	void MaterialEditorPanel::DrawViewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowDockID(m_ViewportDockID);
		ImGui::Begin(m_ViewportName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);

		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x <= 0 || size.y <= 0)
			size = { (float)m_ViewportSize.x, (float)m_ViewportSize.y };

		if ((float)m_ViewportSize.x != size.x || (float)m_ViewportSize.y != size.y)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			m_ViewportSize.x = (uint32_t)ImGui::GetContentRegionAvail().x;
			m_ViewportSize.y = (uint32_t)ImGui::GetContentRegionAvail().y;
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
		ImGuiWindow* window = ImGui::FindWindowByName(m_PanelName.c_str());
		if (window && window->LastFrameActive == (ImGui::GetFrameCount() - 1))
		{
			SK_CORE_ERROR_TAG("UI", "MaterialEditorPanel::Initialize got called twice");
			m_Active = false;
			return;
		}

		SetupWindows();

		m_Camera.SetProjection(m_ViewportSize.y / m_ViewportSize.x, 45.0f, 1.0f, 100.0f);

		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);

		Ref<MeshSource> sphereMeshSource = MeshFactory::GetSphere();
		Ref<Mesh> sphere = ResourceManager::CreateMemoryAsset<Mesh>(sphereMeshSource);
		sphere->GetMaterialTable()->SetMaterial(0, m_Material);

		Entity entity = m_Scene->CreateEntity("Sphere");
		auto& mc = entity.AddComponent<MeshRendererComponent>();
		mc.MeshHandle = sphere->Handle;
		mc.SubmeshIndex = 0;

		m_IsInitialized = true;
	}

	void MaterialEditorPanel::SetupWindows()
	{
		m_ViewportName = fmt::format("Viewport##{}", m_Material->Handle);
		m_SettingsName = fmt::format("Settings##{}", m_Material->Handle);

		ImGuiID tempDockspaceID = m_Material->Handle;
		m_DockspaceID = ImGui::DockBuilderAddNode(tempDockspaceID, ImGuiDockNodeFlags_DockSpace);

		ImGuiDockNode* parentDockspaceNode = ImGui::DockBuilderGetNode(m_ParentDockspaceID);
		ImGui::DockBuilderSetNodeSize(m_DockspaceID, parentDockspaceNode->Size);
		ImGui::DockBuilderSplitNode(m_DockspaceID, ImGuiDir_Left, 0.65, &m_ViewportDockID, &m_SettingsDockID);

		ImGuiDockNode* viewportNode = ImGui::DockBuilderGetNode(m_ViewportDockID);
		viewportNode->SetLocalFlags(ImGuiDockNodeFlags_AutoHideTabBar);

		ImGui::DockBuilderFinish(m_DockspaceID);

		viewportNode = ImGui::DockBuilderGetNode(m_ViewportDockID);
		m_ViewportSize = { viewportNode->Size.x, viewportNode->Size.y };
	}

}
