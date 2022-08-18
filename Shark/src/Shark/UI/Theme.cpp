#include "skpch.h"
#include "Theme.h"

namespace Shark::Theme {

	ImVec4 Colors::ButtonNoBg;
	ImVec4 Colors::ButtonHoveredNoBg;
	ImVec4 Colors::ButtonActiveNoBg;
	ImVec4 Colors::TextInvalidInput;
	ImVec4 Colors::LogInfo;
	ImVec4 Colors::LogWarn;
	ImVec4 Colors::LogError;

	void LoadDark()
	{
		Colors::ButtonNoBg           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		Colors::ButtonHoveredNoBg    = ImVec4(0.31f, 0.31f, 0.31f, 0.50f);
		Colors::ButtonActiveNoBg     = ImVec4(0.39f, 0.39f, 0.39f, 0.50f);

		Colors::TextInvalidInput     = ImVec4(0.80f, 0.30f, 0.10f, 1.00f);

		Colors::LogInfo             = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
		Colors::LogWarn             = ImVec4(0.85f, 0.85f, 0.00f, 1.00f);
		Colors::LogError            = ImVec4(1.00f, 0.20f, 0.10f, 1.00f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowMinSize = ImVec2(16.0f, 16.0f);
		style.IndentSpacing = style.IndentSpacing * 0.5f;
	}

	void LoadLight()
	{
		SK_CORE_ASSERT(false);
	}

	void LoadTheme(const std::filesystem::path& file)
	{
		SK_CORE_ASSERT(false);
	}

}

