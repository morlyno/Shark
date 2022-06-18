#pragma once

#define SK_TEXTURE_EDITOR_PANEL_NEW_UI 0

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Render/EditorCamera.h"

#include "Panels/AssetEditorPanel.h"

#include <imgui.h>

namespace Shark {

	class TextureEditorPanel : public EditorPanel
	{
	public:
		TextureEditorPanel(Ref<Texture2D> sourceTexture);
		~TextureEditorPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown, bool& destroy) override;
		virtual void OnEvent(Event& event) override;

		virtual bool ViewportHovered() const override { return m_ViewportHovered; }
	private:
		void UI_DrawViewport();
		void UI_DrawSettings();

		void SetupWindows();

	private:
		bool m_Active = true;
		bool m_IsFirstFrame = true;

		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_Renderer;
		EditorCamera m_Camera;

		Ref<Texture2D> m_SourceTexture;

		Ref<Texture2D> m_EditTexture;
		Entity m_Entity;

		TextureSpecification m_Specs;

		glm::uvec2 m_ViewportSize = { 1280, 720 };
		bool m_NeedsResize = false;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		ImGuiID m_DockspaceWindowID;
		ImGuiID m_DockspaceID;
		ImGuiID m_ViewportID;
		ImGuiID m_SettingsID;

#if SK_TEXTURE_EDITOR_PANEL_NEW_UI
		FilterMode m_FilterMode = FilterMode::Linear;
		WrapMode m_WrapMode = WrapMode::Repeat;
#endif

		static constexpr std::string_view s_FilterItems[] = { "Neares", "Linear" };
		static constexpr std::string_view s_WrapItems[] = { "Repeat", "Clamp", "Mirror", "Border" };
		static constexpr std::string_view s_FormatItems[] = { "None", "RGBA8", "R32_SINT", "Depth32", "Should never appear" };

	};

}
