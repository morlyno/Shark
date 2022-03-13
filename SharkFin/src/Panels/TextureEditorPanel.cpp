#include "skfpch.h"
#include "TextureEditorPanel.h"

#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"

#include "Shark/UI/UI.h"
#include "Shark/Core/Input.h"
#include "Shark/Utility/Math.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Render/Renderer.h"

#include "Shark/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	TextureEditorPanel::TextureEditorPanel(Ref<Texture2D> sourceTexture)
	{
		m_DockspaceWindowName = fmt::format("Texture Editor##{}", sourceTexture->Handle);
		m_DockspaceName = fmt::format("DockSpace{}", sourceTexture->Handle);
		m_ViewportName = fmt::format("Viewport##{}", sourceTexture->Handle);
		m_SettingsName = fmt::format("Texture Settings##{}", sourceTexture->Handle);

		ImGuiWindow* window = ImGui::FindWindowByName(m_DockspaceWindowName.c_str());
		if (window && window->LastFrameActive == (ImGui::GetFrameCount() - 1))
		{
			m_Active = false;
			return;
		}

		SetupWindows();

		m_SourceTexture = sourceTexture;
		m_EditTexture = ResourceManager::CreateMemoryAsset<Texture2D>(sourceTexture);
		m_Specs = m_EditTexture->GetSpecification();


		m_Scene = Ref<Scene>::Create();
		m_Scene->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene);
		m_Camera.SetProjection((float)m_ViewportSize.x / (float)m_ViewportSize.y, 45, 0.01f, 1000.0f);


		m_Entity = m_Scene->CreateEntity();
		auto& sr = m_Entity.AddComponent<SpriteRendererComponent>();
		sr.TextureHandle = m_EditTexture->Handle;
	}

	TextureEditorPanel::~TextureEditorPanel()
	{
	}

	void TextureEditorPanel::OnUpdate(TimeStep ts)
	{
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
		if (!m_Active)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_DockspaceWindowName.c_str(), &m_Active, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(2);

		ImGui::DockSpace(ImGui::GetID(m_DockspaceName.c_str()));
		ImGui::End();

		UI_DrawViewport();
		UI_DrawSettings();

	}

	void TextureEditorPanel::OnEvent(Event& event)
	{
		if (m_ViewportHovered)
			m_Camera.OnEvent(event);
	}

	void TextureEditorPanel::UI_DrawViewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_ViewportName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);

		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		m_ViewportFocused = ImGui::IsWindowFocused();

		ImVec2 size = ImGui::GetContentRegionAvail();

		if ((float)m_ViewportSize.x != size.x || (float)m_ViewportSize.y != size.y)
		{
			m_ViewportSize = (glm::uvec2)size;
			m_NeedsResize = true;
		}

		// Note(moro): The InvisibleButton prevents imgui form moving the window form anywere but the titlebar
		//             This is currently the only way to do so as ImGuiIO::ConfigWindowsMoveFromTitleBarOnly dosn't work per window
		ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::InvisibleButton("Dummy", size);
		ImGui::SetCursorPos(cursor);

		UI::SetBlend(false);
		ImGui::Image(m_Renderer->GetFinalImage()->GetViewID(), size);
		UI::SetBlend(true);

		ImGui::End();
	}

	void TextureEditorPanel::UI_DrawSettings()
	{
		ImGui::Begin(m_SettingsName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);

		const auto& capabilities = Renderer::GetCapabilities();

#if SK_TEXTURE_EDITOR_PANEL_NEW_UI
		UI::BeginPropertyGrid();
		
		UI::PropertyCombo("Filter Mode", m_FilterMode, s_FilterItems, (uint32_t)std::size(s_FilterItems));
		UI::PropertyCombo("Wrap Mode", m_WrapMode, s_WrapItems, (uint32_t)std::size(s_WrapItems));


		UI::EndProperty();
#else

		UI::BeginPropertyGrid("Settings Grid");

		if (UI::PropertyCustom("Format")) { UI::SpanAvailWith(); int f = (int)m_Specs.Format; if (ImGui::Combo("##Format", &f, s_FormatItems, std::size(s_FormatItems))) { m_Specs.Format = (ImageFormat)f; }; }

		UI::SliderInt("Mip Levels", m_Specs.MipLevels, 1, 0, Renderer::GetCapabilities().MaxMipLeves);
		if (UI::PropertyCustom("Mip Filter")) { UI::SpanAvailWith(); ImGui::Combo("##MipFilter", (int*)&m_Specs.Sampler.Mip, s_FilterItems, std::size(s_FilterItems)); }

		if (UI::PropertyCustom("Filter Mode Min")) { UI::SpanAvailWith(); ImGui::Combo("##MinFilter", (int*)&m_Specs.Sampler.Min, s_FilterItems, std::size(s_FilterItems)); }
		if (UI::PropertyCustom("Filter Mode Mag")) { UI::SpanAvailWith(); ImGui::Combo("##MagFilter", (int*)&m_Specs.Sampler.Mag, s_FilterItems, std::size(s_FilterItems)); }

		if (UI::PropertyCustom("Wrap Mode U")) { UI::SpanAvailWith(); ImGui::Combo("##AddressU", (int*)&m_Specs.Sampler.Wrap.U, s_WrapItems, std::size(s_WrapItems)); }
		if (UI::PropertyCustom("Wrap Mode V")) { UI::SpanAvailWith(); ImGui::Combo("##AddressV", (int*)&m_Specs.Sampler.Wrap.V, s_WrapItems, std::size(s_WrapItems)); }
		if (UI::PropertyCustom("Wrap Mode W")) { UI::SpanAvailWith(); ImGui::Combo("##AddressW", (int*)&m_Specs.Sampler.Wrap.W, s_WrapItems, std::size(s_WrapItems)); }


		UI::Checkbox("Anisotropy", m_Specs.Sampler.Anisotropy);
		UI::DragInt("Max Anisotropy", m_Specs.Sampler.MaxAnisotropy, 0, 0, capabilities.MaxAnisotropy);

		UI::DragFloat("LODBias", m_Specs.Sampler.LODBias, 0.0f, capabilities.MinLODBias, capabilities.MaxLODBias);

		UI::ColorEdit("Border Color", m_Specs.Sampler.BorderColor);

		UI::EndProperty();

		ImGui::Separator();

		if (ImGui::Button("Update"))
			m_EditTexture->Set(m_Specs, m_SourceTexture);

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
		auto& app = Application::Get();
		auto& window = app.GetWindow();

		glm::vec2 windowSize = (glm::vec2)window.GetSize() * 0.45f;
		glm::vec2 windowPos = (glm::vec2)window.GetPos() + (glm::vec2)window.GetSize() * 0.5f - windowSize * 0.5f;

		ImGui::SetNextWindowPos(windowPos);
		ImGui::SetNextWindowSize(windowSize);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_DockspaceWindowName.c_str(), &m_Active, ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceID = ImGui::GetID(m_DockspaceName.c_str());
		ImGui::DockSpace(dockspaceID);
		ImGui::End();

		ImGuiID viewportDockID, settingsDockID;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.55f, &viewportDockID, &settingsDockID);

		ImGuiDockNode* node = ImGui::DockBuilderGetNode(viewportDockID);
		node->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;


		ImGui::SetNextWindowDockID(viewportDockID);
		ImGui::Begin(m_ViewportName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::End();

		ImGui::SetNextWindowDockID(settingsDockID);
		ImGui::Begin(m_SettingsName.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);

		UI::BeginProperty(UI::GetID("Settings Grid"));
		ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch, 0.5f);
		UI::EndProperty();

		ImGui::End();

		m_ViewportSize = (glm::uvec2)ImGui::DockBuilderGetNode(viewportDockID)->Size;
	}

}
