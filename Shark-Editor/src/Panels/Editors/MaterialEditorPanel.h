#pragma once

#include "Shark/Render/Material.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

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
		bool IsReadonly() const { return m_Readonly; }

		void SetName(const std::string& name) { m_Name = name; }
		void SetMaterial(AssetHandle assetHandle) { m_MaterialHandle = assetHandle; }
		AssetHandle GetMaterial() const { return m_MaterialHandle; }

	private:
		std::string m_Name;
		AssetHandle m_MaterialHandle = AssetHandle::Invalid;

		std::string m_RenameBuffer;
		bool m_IsRenaming = false;

		bool m_Readonly = false;
	};

	class MaterialPanel : public Panel
	{
	public:
		MaterialPanel();
		~MaterialPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> context) override { m_Context = context; }

		static const char* GetStaticID() { return "MaterialPanel"; }
		virtual const char* GetPanelID() const { return GetStaticID(); }
	private:
		std::string GetMaterialName(AssetHandle handle) const;

	private:
		Scope<MaterialEditor> m_MaterialEditor = nullptr;

		Ref<Scene> m_Context;
		Entity m_SelectedEntity;
	};

	class MaterialEditorPanel : public EditorPanel
	{
	public:
		MaterialEditorPanel(const std::string& panelName, const AssetMetaData& metadata);
		~MaterialEditorPanel();

		virtual void SetAsset(const AssetMetaData& metadata) override;
		virtual AssetHandle GetAsset() const override { return m_MaterialHandle; }

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
