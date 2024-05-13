#include "skpch.h"
#include "Theme.h"

namespace Shark::Theme {

	static void LoadDarkImGuiColors()
	{
		ImVec4* colors                            = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text]                     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]                 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_ChildBg]                  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
		colors[ImGuiCol_PopupBg]                  = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
		colors[ImGuiCol_Border]                   = ImVec4(0.05f, 0.05f, 0.05f, 0.75f);
		colors[ImGuiCol_BorderShadow]             = ImVec4(0.08f, 0.08f, 0.08f, 0.50f);
		colors[ImGuiCol_FrameBg]                  = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]           = ImVec4(0.08f, 0.08f, 0.08f, 0.40f);
		colors[ImGuiCol_FrameBgActive]            = ImVec4(0.16f, 0.16f, 0.16f, 0.59f);
		colors[ImGuiCol_TitleBg]                  = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_TitleBgActive]            = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]         = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
		colors[ImGuiCol_MenuBarBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark]                = ImVec4(0.01f, 0.66f, 0.04f, 1.00f);
		colors[ImGuiCol_SliderGrab]               = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Button]                   = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_ButtonHovered]            = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
		colors[ImGuiCol_ButtonActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_Header]                   = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered]            = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive]             = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_Separator]                = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
		colors[ImGuiCol_SeparatorActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ResizeGrip]               = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab]                      = ImVec4(0.24f, 0.24f, 0.24f, 0.00f);
		colors[ImGuiCol_TabHovered]               = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TabActive]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TabUnfocused]             = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
		colors[ImGuiCol_TabUnfocusedActive]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_DockingPreview]           = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg]           = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines]                = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]         = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]            = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]     = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]            = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]        = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		colors[ImGuiCol_TableBorderLight]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableRowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]            = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg]           = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget]           = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]    = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]         = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

#if 0
		// New Colors to Test
		colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
#endif

#if 1
		// New Colors to Test
		colors[ImGuiCol_Text]               = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
		colors[ImGuiCol_WindowBg]           = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_Border]             = ImVec4(0.09f, 0.09f, 0.09f, 0.75f);
		colors[ImGuiCol_BorderShadow]       = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
		colors[ImGuiCol_FrameBg]            = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBg]            = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgActive]      = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.10f, 0.27f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.08f, 0.21f, 1.00f);
#endif

		// Even more to test
		colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);

#if TODO
		SK_CORE_VERIFY(false, "Test colors");
#endif
	}
	
	static void LoadDarkThemeColors()
	{
		Colors::WindowBgLight      = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

		Colors::ButtonNoBg         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		Colors::ButtonHoveredNoBg  = ImVec4(0.31f, 0.31f, 0.31f, 0.50f);
		Colors::ButtonActiveNoBg   = ImVec4(0.39f, 0.39f, 0.39f, 0.50f);

		Colors::ButtonDark         = ImGui::ColorConvertU32ToFloat4(0xFF181818 /*0x181818FF*/); // ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		Colors::ButtonActiveDark   = ImGui::ColorConvertU32ToFloat4(0xFF212121 /*0x212121FF*/); // ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		Colors::ButtonHoveredDark  = ImGui::ColorConvertU32ToFloat4(0xFF1B1B1B /*0x1B1B1BFF*/); // ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		Colors::TextDark           = ImGui::ColorConvertU32ToFloat4(0xFF898989 /*0x898989FF*/); // ImVec4(0.700f, 0.700f, 0.700f, 1.000f);

		Colors::TextInvalidInput   = ImVec4(0.80f, 0.30f, 0.10f, 1.00f);

		Colors::PropertyField      = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		Colors::InfoField          = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		Colors::BorderColored      = ImVec4(0.23f, 0.04f, 0.36f, 0.75f);
		Colors::BorderColoredWeak  = ImVec4(0.23f, 0.04f, 0.36f, 0.50f);
		Colors::ShadowColored      = ImVec4(0.23f, 0.04f, 0.36f, 0.50f);

		Colors::Colored            = ImVec4(0.200f, 0.100f, 0.270f, 1.000f);
		Colors::ColoredLight       = ImVec4(0.264f, 0.185f, 0.345f, 1.000f);

		Colors::LogTrace           = ImGui::ColorConvertU32ToFloat4(0xFFB3833E /*0x3E83B3FF*/); // ImVec4(0.10f, 0.10f, 0.50f, 1.00f);
		Colors::LogInfo            = ImGui::ColorConvertU32ToFloat4(0xFF1AB333 /*0x33B31AFF*/); // ImVec4(0.20f, 0.70f, 0.10f, 1.00f);
		Colors::LogWarn            = ImGui::ColorConvertU32ToFloat4(0xFF00B1D9 /*0xD9B100FF*/); // ImVec4(0.85f, 0.85f, 0.00f, 1.00f);
		Colors::LogError           = ImGui::ColorConvertU32ToFloat4(0xFF1A33CC /*0xCC331AFF*/); // ImVec4(0.80f, 0.20f, 0.10f, 1.00f);
		Colors::LogCritical        = ImGui::ColorConvertU32ToFloat4(0xFF1A26F2 /*0xF2261AFF*/); // ImVec4(0.95f, 0.15f, 0.10f, 1.00f);
		Colors::LogTimeColor       = ImGui::ColorConvertU32ToFloat4(0xFF929292 /*0x929292FF*/); // ImVec4(0.08f, 0.55f, 0.87f, 1.00f);
	}

	void LoadDark()
	{
		LoadDarkImGuiColors();
		LoadDarkThemeColors();

		ImGuiStyle& style     = ImGui::GetStyle();
		style.WindowMinSize   = ImVec2(16.0f, 16.0f);
		style.IndentSpacing   = style.IndentSpacing * 0.5f;
		style.FrameBorderSize = 1.0f;
		style.FrameRounding   = 0.0f;//3.0f;
		style.GrabRounding    = 0.0f;//2.0f;
		style.PopupRounding   = 0.0f;//3.0f;
		style.WindowRounding  = 0.0f;//6.0f;
		style.ChildRounding   = 0.0f;//6.0f;
	}

	void LoadLight()
	{
		SK_NOT_IMPLEMENTED();
	}

	void LoadTheme(const std::filesystem::path& file)
	{
		SK_NOT_IMPLEMENTED();
	}

	void DrawThemeEditor(bool& shown)
	{
		if (!shown)
			return;

		ImGui::Begin("Theme Editor", &shown);

		if (ImGui::BeginTabBar("##tabs"))
		{
			if (ImGui::BeginTabItem("Colors"))
			{
				ImGui::ColorEdit4("WindowBgLight", (float*)&Colors::WindowBgLight);
				ImGui::ColorEdit4("ButtonNoBg", (float*)&Colors::ButtonNoBg);
				ImGui::ColorEdit4("ButtonHoveredNoBg", (float*)&Colors::ButtonHoveredNoBg);
				ImGui::ColorEdit4("ButtonActiveNoBg", (float*)&Colors::ButtonActiveNoBg);

				ImGui::ColorEdit4("ButtonDark", (float*)&Colors::ButtonDark);
				ImGui::ColorEdit4("ButtonHoveredDark", (float*)&Colors::ButtonHoveredDark);
				ImGui::ColorEdit4("ButtonActiveDark", (float*)&Colors::ButtonActiveDark);
				ImGui::ColorEdit4("TextDark", (float*)&Colors::TextDark);

				ImGui::ColorEdit4("TextInvalidInput", (float*)&Colors::TextInvalidInput);
				ImGui::ColorEdit4("PropertyField", (float*)&Colors::PropertyField);
				ImGui::ColorEdit4("InfoField", (float*)&Colors::InfoField);
				ImGui::ColorEdit4("BorderColored", (float*)&Colors::BorderColored);
				ImGui::ColorEdit4("BorderColoredWeak", (float*)&Colors::BorderColoredWeak);
				ImGui::ColorEdit4("ShadowColored", (float*)&Colors::ShadowColored);
				ImGui::ColorEdit4("Colored", (float*)&Colors::Colored);
				ImGui::ColorEdit4("ColoredLight", (float*)&Colors::ColoredLight);
				ImGui::ColorEdit4("LogTrace", (float*)&Colors::LogTrace);
				ImGui::ColorEdit4("LogInfo", (float*)&Colors::LogInfo);
				ImGui::ColorEdit4("LogWarn", (float*)&Colors::LogWarn);
				ImGui::ColorEdit4("LogError", (float*)&Colors::LogError);
				ImGui::ColorEdit4("LogCritical", (float*)&Colors::LogCritical);
				ImGui::ColorEdit4("ClockColor", (float*)&Colors::LogTimeColor);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Style"))
			{
				ImGuiStyle& style = ImGui::GetStyle();
				ImGui::DragFloat("Frame Border", &style.FrameBorderSize, 0.1f, 0.0f, FLT_MAX);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		if (ImGui::Button("Reset"))
			LoadDarkThemeColors();
		ImGui::End();
	}

}

