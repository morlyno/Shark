#include "skfpch.h"
#include "TextureEditorPanel.h"

#include "Shark/Scene/Components.h"

#include "Shark/UI/UI.h"
#include "Shark/Input/Input.h"
#include "Shark/Math/Math.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Core/Application.h"

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(const std::string& panelName, ImGuiID parentDockspaceID, Ref<Texture2D> sourceTexture)
		: EditorPanel(panelName, parentDockspaceID)
	{
		SetTexture(sourceTexture);
	}

	TextureEditorPanel::~TextureEditorPanel()
	{
	}

	void TextureEditorPanel::SetTexture(Ref<Texture2D> sourceTexture)
	{
		m_SourceTexture = sourceTexture;
		m_IsFirstFrame = true;
		m_Initialized = false;
	}

	void TextureEditorPanel::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (!m_Initialized)
			return;

		if (!m_Active)
			return;

		if (m_NeedsResize && m_ViewportSize.x != 0 && m_ViewportSize.y != 0)
		{
			ReCalcCamera();
			m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_Renderer->Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_NeedsResize = false;
		}

		m_Scene->OnRender(m_Renderer, m_Camera.GetProjection());
	}

	void TextureEditorPanel::OnImGuiRender(bool& shown, bool& destroy)
	{
		SK_PROFILE_FUNCTION();

		if (!m_SourceTexture)
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
		ImGui::Begin(m_PanelName.c_str(), &m_Active, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(2);

		ImGui::DockSpace(m_DockspaceID);
		ImGui::End();

		UI_DrawViewport();
		UI_DrawSettings();

	}

	void TextureEditorPanel::UI_DrawViewport()
	{
		SK_PROFILE_FUNCTION();

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

		// Note(moro): The InvisibleButton prevents imgui form moving the window form anywere but the titlebar
		//             This is currently the only way to do so as ImGuiIO::ConfigWindowsMoveFromTitleBarOnly dosn't work per window
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::InvisibleButton("Dummy", size);
		ImGui::SetCursorPos(cursor);
		ImGui::Image(m_Renderer->GetFinalImage()->GetViewID(), size);

		ImGui::End();
	}

	void TextureEditorPanel::UI_DrawSettings()
	{
		SK_PROFILE_FUNCTION();

		ImGui::SetNextWindowDockID(m_SettingsDockID);
		ImGui::Begin(m_SettingsName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);

		const auto& capabilities = Renderer::GetCapabilities();

#if SK_TEXTURE_EDITOR_PANEL_NEW_UI
		UI::BeginPropertyGrid();
		
		UI::PropertyCombo("Filter Mode", m_FilterMode, s_FilterItems, (uint32_t)std::size(s_FilterItems));
		UI::PropertyCombo("Wrap Mode", m_WrapMode, s_WrapItems, (uint32_t)std::size(s_WrapItems));


		UI::EndProperty();
#else

		ImGui::TextDisabled("Note: UVs?");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("Currently don't know if UVs should be saved in Texture or it should be part of SpriteRendererComponent");
			ImGui::Text("UVs in Texture could increas the Texture Asset count");
			ImGui::Text("Best way probably only in Material with would be the same as SpriteRendererComponent");
			ImGui::EndTooltip();
		}

		UI::BeginControlsGrid();

		UI::ControlCombo("Format", (uint16_t&)m_Specification.Format, s_FormatItems, (uint32_t)std::size(s_FormatItems));
		UI::Control("Generate Mips", m_Specification.GenerateMips);
		UI::ControlCombo("Filter", s_FilterItems[(uint16_t)m_Specification.Sampler.Filter], [&index = (uint16_t&)m_Specification.Sampler.Filter]()
		{
			bool changed = false;

			for (uint16_t i = 1; i < std::size(s_FilterItems); i++)
			{
				std::string_view currentItem = s_FilterItems[i];
				if (ImGui::Selectable(currentItem.data(), i == index))
				{
					index = i;
					changed = true;
				}
			}
			return changed;
		});
		UI::ControlCombo("Wrap Mode", s_FilterItems[(uint16_t)m_Specification.Sampler.Wrap], [&index = (uint16_t&)m_Specification.Sampler.Wrap]()
		{
			bool changed = false;

			for (uint16_t i = 1; i < std::size(s_WrapItems); i++)
			{
				std::string_view currentItem = s_WrapItems[i];
				if (ImGui::Selectable(currentItem.data(), i == index))
				{
					index = i;
					changed = true;
				}
			}
			return changed;
		});

		UI::Control("Anisotropy", m_Specification.Sampler.Anisotropy);
		UI::Control("Max Anisotropy", m_Specification.Sampler.MaxAnisotropy, 0, 0, capabilities.MaxAnisotropy);

		UI::EndControls();

		ImGui::Separator();

		if (ImGui::Button("Update"))
		{
			m_EditTexture->GetSpecificationMutable() = m_Specification;
			m_EditTexture->Invalidate();
			m_EditTexture->GetImage()->UploadImageData(m_SourceTexture->GetImage());
			if (m_Specification.GenerateMips)
				Renderer::GenerateMips(m_EditTexture->GetImage());
		}

		if (ImGui::Button("Reset"))
			m_Specification = m_SourceTexture->GetSpecification();

		if (ImGui::Button("Finish"))
		{
			m_SourceTexture->GetSpecificationMutable() = m_Specification;
			m_SourceTexture->Invalidate();
			m_SourceTexture->GetImage()->UploadImageData(m_EditTexture->GetImage());

			ResourceManager::SaveAsset(m_SourceTexture->Handle);
			ResourceManager::ReloadAsset(m_SourceTexture->Handle);
		}
#endif

		ImGui::End();
	}

	void TextureEditorPanel::Initialize()
	{
		// Check if current texture has allready been used
		ImGuiWindow* window = ImGui::FindWindowByName(m_PanelName.c_str());
		if (window && window->LastFrameActive == (ImGui::GetFrameCount() - 1))
		{
			SK_CORE_ERROR_TAG("UI", "TextureEditorPanel::Initailize got called twice");
			m_Active = false;
			return;
		}

		m_ViewportName = fmt::format("Viewport##{}", m_SourceTexture->Handle);
		m_SettingsName = fmt::format("Settings##{}", m_SourceTexture->Handle);

		SetupWindows();

		m_EditTexture = ResourceManager::CreateMemoryAsset<Texture2D>(m_SourceTexture->GetSpecification().Sampler, m_SourceTexture->GetImage(), false);
		m_Specification = m_EditTexture->GetSpecification();

		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);

		ReCalcCamera();

		m_Entity = m_Scene->CreateEntity();
		auto& sr = m_Entity.AddComponent<SpriteRendererComponent>();
		sr.TextureHandle = m_EditTexture->Handle;

		m_Initialized = true;
	}

	void TextureEditorPanel::SetupWindows()
	{
		ImGuiID tempDockspaceID = m_SourceTexture->Handle;
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

	void TextureEditorPanel::ReCalcCamera()
	{
		const float aspectRatio = (float)m_ViewportSize.x / (float)m_ViewportSize.y;
		const float zoom = 0.55f;
		m_Camera = glm::ortho(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom);
	}

}