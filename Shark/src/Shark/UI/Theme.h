#pragma once

#include <imgui.h>

namespace Shark::Theme {

	void LoadDark();
	void LoadLight();
	void LoadTheme(const std::filesystem::path& file);
	void DrawThemeEditor(bool& shown);

	struct Colors
	{
		inline static ImVec4 WindowBgLight;

		inline static ImVec4 ButtonNoBg;
		inline static ImVec4 ButtonHoveredNoBg;
		inline static ImVec4 ButtonActiveNoBg;

		inline static ImVec4 TextInvalidInput;

		inline static ImVec4 PropertyField;
		inline static ImVec4 InfoField;
		inline static ImVec4 BorderColored;
		inline static ImVec4 BorderColoredWeak;

		inline static ImVec4 ShadowColored;

		// Console
		inline static ImVec4 LogTrace;
		inline static ImVec4 LogInfo;
		inline static ImVec4 LogWarn;
		inline static ImVec4 LogError;
		inline static ImVec4 LogCritical;
		inline static ImVec4 LogTimeColor;
	};

}
