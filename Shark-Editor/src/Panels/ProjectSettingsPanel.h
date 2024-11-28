#pragma once

#include "Shark/UI/UICore.h"
#include "Shark/UI/TextFilter.h"
#include "Panel.h"

#include <magic_enum_containers.hpp>

namespace Shark {

	class ProjectSettingsPanel : public Panel
	{
	private:
		enum class ActiveContext
		{
			General, Scripting, Physics, Log
		};

	public:
		ProjectSettingsPanel(const std::string& panelName, Ref<Project> project);
		~ProjectSettingsPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnProjectChanged(Ref<Project> project) override;

	private:
		void DrawGeneralSettings();
		void DrawScriptingSettings();
		void DrawPhysicsSettings();
		void DrawLogSettings();

		void LogChanges();
		void RenameAndSaveProject();

	private:
		Ref<Project> m_Project;
		ActiveContext m_ActiveContext = ActiveContext::General;
		ProjectConfig m_TempConfig;
		bool m_ConfigDirty = false;
		magic_enum::containers::array<ActiveContext, bool> m_DirtyMenu;

		bool m_Focused = false;
		ImGuiID m_UnsavedSettingsID = UI::GenerateUniqueID();

		UI::TextFilter m_LogFilter;
	};

}
