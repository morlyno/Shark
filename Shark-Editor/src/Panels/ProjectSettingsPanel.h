#pragma once

#include "Shark/Core/Enum.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/TextFilter.h"

#include "Panel.h"

namespace Shark {

	class ProjectSettingsPanel : public Panel
	{
	private:
		enum class ActiveContext
		{
			General, Scripting, Physics, Log
		};

	public:
		ProjectSettingsPanel(Ref<ProjectConfig> projectConfig);
		~ProjectSettingsPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnProjectChanged(const Ref<ProjectConfig>& projectConfig) override;

		static const char* GetStaticID() { return "ProjectSettingsPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		void DrawGeneralSettings();
		void DrawScriptingSettings();
		void DrawPhysicsSettings();
		void DrawLogSettings();

		void LogChanges();
		void RenameAndSaveProject();

	private:
		Ref<ProjectConfig> m_ProjectConfig;
		ActiveContext m_ActiveContext = ActiveContext::General;
		Ref<ProjectConfig> m_TempConfig;

		bool m_ConfigDirty = false;
		Enum::Array<ActiveContext, bool> m_DirtyMenu;

		bool m_Focused = false;
		ImGuiID m_UnsavedSettingsID = UI::GenerateUniqueID();

		UI::TextFilter m_LogFilter;
	};

}
