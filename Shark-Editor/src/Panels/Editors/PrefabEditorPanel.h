#pragma once

#include "Shark/Asset/AssetTypes.h"
#include "Shark/Render/EditorCamera.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {
	class Prefab;
	class SceneRenderer;
	class SceneHierarchyPanel;

	struct AssetMetaData;
}

namespace Shark {

	class PrefabEditorPanel : public EditorPanel
	{
	public:
		PrefabEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~PrefabEditorPanel();

		virtual void DockWindow(ImGuiID dockspaceID) override;

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& showWindow) override;
		virtual void OnEvent(Event& event) override;

	private:
		void SetAsset(const AssetMetaData& metadata);
		void PrepareForSerialization() const;

	private:
		AssetHandle m_Handle;
		Ref<Prefab> m_Prefab;

		Ref<SceneHierarchyPanel> m_HierarchyPanel = nullptr;
		Ref<SceneRenderer> m_SceneRenderer;
		EditorCamera m_EditorCamera;

		ImGuiID m_DockSpaceID = 0;
		bool m_DockWindow = false;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_NeedsResize = false;

		AssetHandle m_DefaultEnvironment;
		bool m_UseDefaultSky = true;
	};

}
