#include "skfpch.h"
#include "TextureEditorPanel.h"

#include "Shark/Scene/Components.h"

#include "Shark/UI/UI.h"
#include "Shark/Input/Input.h"
#include "Shark/Math/Math.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(const char* panelName, Ref<Texture2D> sourceTexture)
		: EditorPanel(PanelName)
	{
		SK_PROFILE_FUNCTION();

		m_DockspaceWindowID = UI::GetID(fmt::format("DockspaceWindow{}", sourceTexture->Handle));
		m_DockspaceID       = UI::GetID(fmt::format("DockSpace{}", sourceTexture->Handle));
		m_ViewportID        = UI::GetID(fmt::format("Viewport{}", sourceTexture->Handle));
		m_SettingsID        = UI::GetID(fmt::format("Settings{}", sourceTexture->Handle));

		// Check if current texture has allready been used
		ImGuiWindow* window = ImGui::FindWindowByID(m_DockspaceWindowID);
		if (window && window->LastFrameActive == (ImGui::GetFrameCount() - 1))
		{
			m_Active = false;
			return;
		}

		SetupWindows();
		ImGui::FocusWindow(ImGui::FindWindowByID(m_ViewportID));

		m_SourceTexture = sourceTexture;

		m_EditTexture = ResourceManager::CreateMemoryAsset<Texture2D>();
		m_EditTexture->GetSpecificationMutable() = m_SourceTexture->GetSpecification();
		m_EditTexture->Invalidate();
		m_EditTexture->GetImage()->SetImageData(m_SourceTexture->GetImage());

		m_Specs = m_EditTexture->GetSpecification();


		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);

		ReCalcCamera();

		m_Entity = m_Scene->CreateEntity();
		auto& sr = m_Entity.AddComponent<SpriteRendererComponent>();
		sr.TextureHandle = m_EditTexture->Handle;
	}

	TextureEditorPanel::~TextureEditorPanel()
	{
		SK_PROFILE_FUNCTION();
	}

	void TextureEditorPanel::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

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

		if (!m_Active)
		{
			shown = false;
			destroy = true;
			return;
		}

		if (m_IsFirstFrame)
		{
			m_IsFirstFrame = false;
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginEx(PanelName, m_DockspaceWindowID, &m_Active, ImGuiWindowFlags_NoSavedSettings);
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
		ImGui::BeginEx("Viewport", m_ViewportID, nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);

		const ImVec2 size = ImGui::GetContentRegionAvail();
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

		ImGui::BeginEx("Settings", m_SettingsID, nullptr, ImGuiWindowFlags_NoSavedSettings);

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

		UI::ControlCombo("Format", (uint16_t&)m_Specs.Format, s_FormatItems, (uint32_t)std::size(s_FormatItems));
		UI::Control("Generate Mips", m_Specs.GenerateMips);
		UI::ControlCombo("Filter", (uint16_t&)m_Specs.Sampler.Filter, s_FilterItems, (uint32_t)std::size(s_FilterItems));
		UI::ControlCombo("Wrap Mode", (uint16_t&)m_Specs.Sampler.Wrap, s_WrapItems, (uint32_t)std::size(s_WrapItems));

		UI::Control("Anisotropy", m_Specs.Sampler.Anisotropy);
		UI::Control("Max Anisotropy", m_Specs.Sampler.MaxAnisotropy, 0, 0, capabilities.MaxAnisotropy);

		UI::EndControls();

		ImGui::Separator();

		if (ImGui::Button("Update"))
		{
			m_EditTexture->GetSpecificationMutable() = m_Specs;
			m_EditTexture->Invalidate();
			m_EditTexture->GetImage()->SetImageData(m_SourceTexture->GetImage());
			if (m_Specs.GenerateMips)
				Renderer::GenerateMips(m_EditTexture->GetImage());
		}

		if (ImGui::Button("Reset"))
			m_Specs = m_SourceTexture->GetSpecification();

		if (ImGui::Button("Finish"))
		{
			m_SourceTexture->GetSpecificationMutable() = m_Specs;
			m_SourceTexture->Invalidate();
			m_SourceTexture->GetImage()->SetImageData(m_EditTexture->GetImage());

			ResourceManager::SaveAsset(m_SourceTexture->Handle);
			ResourceManager::ReloadAsset(m_SourceTexture->Handle);
		}
#endif

		ImGui::End();
	}

	void TextureEditorPanel::SetupWindows()
	{
		SK_PROFILE_FUNCTION();

		if (!ImGui::FindWindowByID(m_DockspaceWindowID))
		{
			auto& app = Application::Get();
			auto& window = app.GetWindow();

			const float windowRatio = 0.55f;
			const float dockSplittRatio = 0.65f;

			const glm::vec2 windowSize = (glm::vec2)window.GetSize() * windowRatio;
			const glm::vec2 windowPos = (glm::vec2)window.GetPosition() + (glm::vec2)window.GetSize() * 0.5f - windowSize * 0.5f;

			ImGui::SetNextWindowPos({ windowPos.x, windowPos.y });
			ImGui::SetNextWindowSize({ windowSize.x, windowSize.y });

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginEx("Texture Editor", m_DockspaceWindowID, nullptr, ImGuiWindowFlags_NoSavedSettings);
			ImGui::PopStyleVar(3);
			//ImGui::DockSpace(m_DockspaceID, windowSize);
			ImGui::End();

			ImGui::BeginEx("Viewport", m_ViewportID, nullptr, ImGuiWindowFlags_NoSavedSettings);
			ImGui::End();

			ImGui::BeginEx("Settings", m_SettingsID, nullptr, ImGuiWindowFlags_NoSavedSettings);
			ImGui::End();

			ImGuiWindow* dock = ImGui::FindWindowByID(m_DockspaceWindowID);
			ImGuiWindow* viewport = ImGui::FindWindowByID(m_ViewportID);
			ImGuiWindow* settings = ImGui::FindWindowByID(m_SettingsID);

			ImGui::DockBuilderAddNode(m_DockspaceID, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(m_DockspaceID, { windowSize.x, windowSize.y });
			ImGui::DockBuilderSetNodePos(m_DockspaceID, { windowPos.x, windowPos.y });
			ImGui::SetWindowDock(dock, m_DockspaceID, ImGuiCond_Always);

			ImGuiDockNode* dockspaceNode = ImGui::DockBuilderGetNode(m_DockspaceID);
			ImGui::DockNodeAddWindow(dockspaceNode, dock, true);


			ImGuiID dockViewport, dockSettings;
			ImGui::DockBuilderSplitNode(m_DockspaceID, ImGuiDir_Left, dockSplittRatio, &dockViewport, &dockSettings);

			ImGuiDockNode* viewportNode = ImGui::DockBuilderGetNode(dockViewport);
			ImGuiDockNode* settingsNode = ImGui::DockBuilderGetNode(dockSettings);

			viewportNode->SetLocalFlags(ImGuiDockNodeFlags_NoTabBar);

			ImGui::SetWindowDock(viewport, dockViewport, ImGuiCond_Always);
			ImGui::SetWindowDock(settings, dockSettings, ImGuiCond_Always);
			ImGui::DockNodeAddWindow(viewportNode, viewport, false);
			ImGui::DockNodeAddWindow(settingsNode, settings, true);
			
			const float tabbarHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::DockBuilderSetNodeSize(dockViewport, viewportNode->Size - ImVec2(0.0f, tabbarHeight));
		}

		ImGuiDockNode* viewportNode = ImGui::FindWindowByID(m_ViewportID)->DockNode;
		ImRect rect = viewportNode->Rect();
		m_ViewportSize.x = (uint32_t)rect.GetWidth();
		m_ViewportSize.y = (uint32_t)rect.GetHeight();
	}

	void TextureEditorPanel::ReCalcCamera()
	{
		const float aspectRatio = (float)m_ViewportSize.x / (float)m_ViewportSize.y;
		const float zoom = 0.55f;
		m_Camera = glm::ortho(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom);
	}

}
