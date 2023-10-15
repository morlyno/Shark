#pragma once

#include "Shark/Render/Material.h"
#include "Shark/Render/SceneRenderer.h"
#include "Shark/Scene/Scene.h"

#include "Panels/AssetEditorPanel.h"

namespace Shark {

	class MaterialEditor
	{
	public:
		MaterialEditor(const std::string& name, Ref<MaterialAsset> material);

		void Draw();
		void DrawInline();

		void SetReadonly(bool readonly) { m_Readonly = readonly; }

		void SetName(const std::string& name) { m_Name = name; }
		void SetMaterial(Ref<MaterialAsset> material) { m_Material = material; }

	private:
		std::string m_Name;
		Ref<MaterialAsset> m_Material;

		bool m_Readonly = false;
	};

	class MaterialEditorPanel : public EditorPanel
	{
	public:
		MaterialEditorPanel(const std::string& panelName, ImGuiID parentDockspaceID, Ref<MaterialAsset> material);

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown, bool& destroy) override;
		virtual void OnEvent(Event& event) override;

		void DrawSettings();
		void DrawViewport();

	private:
		void Initialize();
		void SetupSceneAndRenderer();
		void SetupWindows();

	private:
		bool m_Tiny = false;

		bool m_IsFirstFrame = true;
		bool m_IsInitialized = false;
		bool m_Active = true;

		Scope<MaterialEditor> m_MaterialEditor;
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
