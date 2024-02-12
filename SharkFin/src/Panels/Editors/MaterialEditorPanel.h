#pragma once

#include "Shark/Render/Material.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Scene/Scene.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class MaterialEditor
	{
	public:
		MaterialEditor() = default;
		MaterialEditor(const std::string& name, AssetHandle assetHandle);

		void Draw();
		void DrawInline();

		void SetReadonly(bool readonly) { m_Readonly = readonly; }

		void SetName(const std::string& name) { m_Name = name; }
		void SetMaterial(AssetHandle assetHandle) { m_MaterialHandle = assetHandle; }

	private:
		std::string m_Name;
		AssetHandle m_MaterialHandle = AssetHandle::Invalid;

		std::string m_RenameBuffer;
		bool m_IsRenaming = false;

		bool m_Readonly = false;
	};

	class MaterialEditorPanel : public EditorPanel
	{
	public:
		MaterialEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~MaterialEditorPanel();

		virtual void SetAsset(const AssetMetaData& metadata) override;
		virtual void DockWindow(ImGuiID dockspace) override;

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown, bool& destroy) override;

	private:
		bool m_Active = true;

		ImGuiID m_DockWindowID = 0;
		bool m_DockWindow = false;

		Scope<MaterialEditor> m_MaterialEditor = nullptr;
		AssetHandle m_MaterialHandle;

		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		bool m_NeedsResize = false;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_Renderer;
		AssetHandle m_Sphere;

		std::string m_ViewportName;
		std::string m_SettingsName;
	};

}
