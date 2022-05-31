#pragma once

#include <imgui.h>

namespace Shark::Theme {

	void LoadDark();
	void LoadLight();
	void LoadTheme(const std::filesystem::path& file);

	struct Colors
	{
		static ImVec4 ButtonNoBg;
		static ImVec4 ButtonHoveredNoBg;
		static ImVec4 ButtonActiveNoBg;

		static ImVec4 TextInvalidInput;

		static ImVec4 LogTrace;
		static ImVec4 LogInfo;
		static ImVec4 LogWarn;
		static ImVec4 LogError;
		static ImVec4 LogCritical;
		static ImVec4 LogDebug;
		static ImVec4 LogCriticalBg;
	};

	const ImVec4& GetLogColor(Log::Level level);

}
