#pragma once

#include "Panel.h"

namespace Shark {

	class ProjectSettingsPanel : public Panel
	{
	public:
		ProjectSettingsPanel(const std::string& panelName, Ref<Project> project);
		~ProjectSettingsPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnProjectChanged(Ref<Project> project) override;

	private:
		Ref<Project> m_Project;
		char m_RenameBuffer[256];
	};

}
