#pragma once

#include "Shark/Editor/Panel.h"

namespace Shark {

	class EditorPanel : public RefCount
	{
	public:
		EditorPanel(const char* panelName) : PanelName(panelName) {}
		virtual ~EditorPanel() = default;

		virtual void OnUpdate(TimeStep ts) {};
		virtual void OnImGuiRender(bool& shown, bool& destroy) {};
		virtual void OnEvent(Event& event) {};

	public:
		const char* PanelName = nullptr;
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
		AssetEditorPanel(const char* panelName);
		virtual ~AssetEditorPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		template<typename T, typename... Args>
		Ref<T> AddEditor(UUID id, const char* panelName, bool shown, Args&&... args)
		{
			if (m_EditorPanels.find(id) != m_EditorPanels.end())
				return nullptr;

			Ref<T> editor = Ref<T>::Create(panelName, std::forward<Args>(args)...);
			m_EditorPanels[id] = { editor, shown, false };
			return editor;
		}

		template<typename T = EditorPanel>
		Ref<T> GetEditor(UUID id)
		{
			return m_EditorPanels.at(id);
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
		std::unordered_map<UUID, EditorPanelEntry> m_EditorPanels;
	};

}	
