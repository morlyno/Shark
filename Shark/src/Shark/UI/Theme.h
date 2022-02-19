#pragma once

#include <imgui.h>

namespace Shark::UI::Theme {

	void LoadDark();
	void LoadLight();
	void LoadTheme(const std::filesystem::path& file);

	struct Color
	{
		ImVec4 ButtonNoBg;
		ImVec4 ButtonHoveredNoBg;
		ImVec4 ButtonActiveNoBg;
	};

	const Color& GetColors();

}
