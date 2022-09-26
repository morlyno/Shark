#pragma once

#include <imgui.h>

namespace Shark::Theme {

	void LoadDark();
	void LoadLight();
	void LoadTheme(const std::filesystem::path& file);
	void DrawThemeEditor(bool& shown);

	struct Colors
	{
		static ImVec4 ButtonNoBg;
		static ImVec4 ButtonHoveredNoBg;
		static ImVec4 ButtonActiveNoBg;

		static ImVec4 TextInvalidInput;

		static ImVec4 PropertyField;
		static ImVec4 InfoField;
		static ImVec4 Border;
		static ImVec4 BorderColored;
		static ImVec4 ShadowColored;

		// Console
		static ImVec4 LogInfo;
		static ImVec4 LogWarn;
		static ImVec4 LogError;
		static ImVec4 LogTimeColor;
	};

}
