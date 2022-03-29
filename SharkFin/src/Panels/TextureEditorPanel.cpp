#include "skfpch.h"
#include "TextureEditorPanel.h"

#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"

#include "Shark/UI/UI.h"
#include "Shark/Core/Input.h"
#include "Shark/Utility/Math.h"
#include "Shark/Utility/Utility.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(Ref<Texture2D> sourceTexture)
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
		m_EditTexture = ResourceManager::CreateMemoryAsset<Texture2D>(sourceTexture);
		m_Specs = m_EditTexture->GetSpecification();


		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);
		m_Camera.SetProjection((float)m_ViewportSize.x / (float)m_ViewportSize.y, 45, 0.01f, 1000.0f);
		m_Camera.SetDistance(1.5f);


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
			SK_CORE_INFO("TextureEditorPanel::OnUpdate Resize");

			m_Camera.Resize((float)m_ViewportSize.x, (float)m_ViewportSize.y);
			m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_Renderer->Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_NeedsResize = false;
		}

		if (m_ViewportFocused)
			m_Camera.OnUpdate(ts);

		m_Scene->OnUpdateEditor(ts);
		m_Scene->OnRenderEditor(m_Renderer, m_Camera);
	}

	void TextureEditorPanel::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		if (!m_Active)
			return;

		if (m_IsFirstFrame)
		{
			m_IsFirstFrame = false;
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginEx("Texture Editor", m_DockspaceWindowID, &m_Active, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(2);

		ImGui::DockSpace(m_DockspaceID);
		ImGui::End();

		UI_DrawViewport();
		UI_DrawSettings();

	}

	void TextureEditorPanel::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		if (m_ViewportHovered)
			m_Camera.OnEvent(event);
	}

	void TextureEditorPanel::UI_DrawViewport()
	{
		SK_PROFILE_FUNCTION();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginEx("Viewport", m_ViewportID, nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		m_ViewportFocused = ImGui::IsWindowFocused();

		const ImVec2 size = ImGui::GetContentRegionAvail();
		if ((float)m_ViewportSize.x != size.x || (float)m_ViewportSize.y != size.y)
		{
			SK_CORE_WARN("Resize detected: {} => {}", m_ViewportSize, size);
			m_ViewportSize = (glm::uvec2)size;
			m_NeedsResize = true;
		}

		// Note(moro): The InvisibleButton prevents imgui form moving the window form anywere but the titlebar
		//             This is currently the only way to do so as ImGuiIO::ConfigWindowsMoveFromTitleBarOnly dosn't work per window
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::InvisibleButton("Dummy", size);
		ImGui::SetCursorPos(cursor);

		UI::SetBlend(false);
		ImGui::Image(m_Renderer->GetFinalImage()->GetViewID(), size);
		UI::SetBlend(true);

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

		UI::Control("Format", Utility::EnumToInt(m_Specs.Format), s_FormatItems, (uint32_t)std::size(s_FormatItems));
		UI::Control("Mip Levels", m_Specs.MipLevels, 1, 0, Renderer::GetCapabilities().MaxMipLeves);
		UI::Control("Mip Filter", Utility::EnumToInt(m_Specs.Sampler.Mip), s_FilterItems, std::size(s_FilterItems));
		UI::Control("Mip Mode Min", Utility::EnumToInt(m_Specs.Sampler.Min), s_FilterItems, std::size(s_FilterItems));
		UI::Control("Mip Mode Mag", Utility::EnumToInt(m_Specs.Sampler.Mag), s_FilterItems, std::size(s_FilterItems));

		UI::Control("Wrap Mode U", Utility::EnumToInt(m_Specs.Sampler.Wrap.U), s_WrapItems, std::size(s_WrapItems));
		UI::Control("Wrap Mode V", Utility::EnumToInt(m_Specs.Sampler.Wrap.V), s_WrapItems, std::size(s_WrapItems));
		UI::Control("Wrap Mode W", Utility::EnumToInt(m_Specs.Sampler.Wrap.W), s_WrapItems, std::size(s_WrapItems));

		UI::Control("Anisotropy", m_Specs.Sampler.Anisotropy);
		UI::Control("Max Anisotropy", m_Specs.Sampler.MaxAnisotropy, 0, 0, capabilities.MaxAnisotropy);
		UI::Control("LODBias", m_Specs.Sampler.LODBias, 0.0f, capabilities.MinLODBias, capabilities.MaxLODBias);
		UI::ControlColor("Border Color", m_Specs.Sampler.BorderColor);

		UI::EndControls();

		ImGui::Separator();

		if (ImGui::Button("Update"))
		{
			m_EditTexture->Set(m_Specs, m_SourceTexture);
			if (m_Specs.MipLevels != 1)
				Renderer::GenerateMips(m_EditTexture->GetImage());
		}

		if (ImGui::Button("Reset"))
			m_Specs = m_SourceTexture->GetSpecification();

		if (ImGui::Button("Finish"))
		{
			m_SourceTexture->Set(m_Specs, m_EditTexture);
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

			const float windowRatio = 0.55;
			const float dockSplittRatio = 0.65f;

			const glm::vec2 windowSize = (glm::vec2)window.GetSize() * windowRatio;
			const glm::vec2 windowPos = (glm::vec2)window.GetPos() + (glm::vec2)window.GetSize() * 0.5f - windowSize * 0.5f;

			ImGui::SetNextWindowPos(windowPos);
			ImGui::SetNextWindowSize(windowSize);

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
			ImGui::DockBuilderSetNodeSize(m_DockspaceID, windowSize);
			ImGui::DockBuilderSetNodePos(m_DockspaceID, windowPos);
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
			
			// NOTE(moro): more context to 19.0f soon
			//             19.0f shoud be the height of a nodes tabbar
			const float tabbarHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::DockBuilderSetNodeSize(dockViewport, viewportNode->Size - ImVec2(0.0f, tabbarHeight));
		}

		ImGuiDockNode* viewportNode = ImGui::FindWindowByID(m_ViewportID)->DockNode;
		m_ViewportSize = { viewportNode->Size.x, viewportNode->Size.y };
	}

}
