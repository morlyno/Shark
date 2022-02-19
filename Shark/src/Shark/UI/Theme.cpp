#include "skpch.h"
#include "Theme.h"

namespace Shark::UI::Theme {

	static Color* s_Colors = new Color;

	void LoadDark()
	{
		s_Colors->ButtonNoBg           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		s_Colors->ButtonHoveredNoBg    = ImVec4(0.31f, 0.31f, 0.31f, 0.50f);
		s_Colors->ButtonActiveNoBg     = ImVec4(0.39f, 0.39f, 0.39f, 0.50f);
	}

	void LoadLight()
	{
		SK_CORE_ASSERT(false);
	}

	void LoadTheme(const std::filesystem::path& file)
	{
		SK_CORE_ASSERT(false);
	}

	const Color& GetColors()
	{
		return *s_Colors;
	}

}

