#pragma once

#include "Shark/Editor/Panel.h"
#include "Shark/UI/UI.h"

namespace Shark {

	class EditorPanel : public RefCount
	{
	public:
		EditorPanel(const std::string& panelName, ImGuiID parentDockspaceID)
			: m_PanelName(panelName), m_ParentDockspaceID(parentDockspaceID)
		{}
		virtual ~EditorPanel() = default;

		virtual void OnUpdate(TimeStep ts) {};
		virtual void OnImGuiRender(bool& shown, bool& destroy) {};
		virtual void OnEvent(Event& event) {};

		void SetPanelName(const std::string& name) { m_PanelName = name; }
		const std::string& GetPanelName() const { return m_PanelName; }

	protected:
		std::string m_PanelName;
		ImGuiID m_ParentDockspaceID;
	};

	class AssetEditorPanel : public Panel
	{
	private:
		struct EditorPanelEntry
		{
			Ref<EditorPanel> Editor;
			bool Shown;
			bool Destroy;
		};

	public:
		AssetEditorPanel(const std::string& panelName);
		virtual ~AssetEditorPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		template<typename T, typename... Args>
		Ref<T> AddEditor(UUID id, const std::string& panelName, bool shown, Args&&... args)
		{
			if (m_EditorPanels.find(id) != m_EditorPanels.end())
				return GetEditor<T>(id);

			Ref<T> editor = Ref<T>::Create(panelName, m_DockspaceID, std::forward<Args>(args)...);
			m_EditorPanels[id] = { editor, shown, false };
			return editor;
		}

		template<typename T = EditorPanel>
		Ref<T> GetEditor(UUID id)
		{
			return m_EditorPanels.at(id).Editor.As<T>();
		}

		void RemoveEditor(UUID id)
		{
			m_EditorPanels.erase(id);
		}

		bool HasEditor(UUID id)
		{
			return m_EditorPanels.find(id) != m_EditorPanels.end();
		}

	private:
		void DrawPanels();

	private:
		std::unordered_map<UUID, EditorPanelEntry> m_EditorPanels;
		ImGuiID m_DockspaceID;
	};

}	
