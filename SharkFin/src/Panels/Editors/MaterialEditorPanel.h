#pragma once

#include "Shark/Render/Material.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Scene/Scene.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class MaterialEditorPanel : public EditorPanel
	{
	public:
		MaterialEditorPanel(const std::string& panelName, ImGuiID parentDockspaceID, Ref<MaterialAsset> material);

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown, bool& destroy) override;

		void DrawSettings();
		void DrawViewport();

	private:
		void Initialize();
		void SetupWindows();

	private:
		bool m_IsInitialized = false;
		bool m_Active = true;
		bool m_IsFirstFrame = true;
		Ref<MaterialAsset> m_Material;

		glm::vec2 m_ViewportSize;
		bool m_NeedsResize = false;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_Renderer;
		EditorCamera m_Camera;

		std::string m_ViewportName;
		std::string m_SettingsName;

		ImGuiID m_DockspaceID = 0;
		ImGuiID m_ViewportDockID = 0;
		ImGuiID m_SettingsDockID = 0;
	};

}
