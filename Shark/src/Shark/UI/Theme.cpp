#include "skpch.h"
#include "Theme.h"

namespace Shark::UI::Colors {

	static void LoadDarkImGuiColors()
	{
#if 0
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

		colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
#endif

	}
	
	void LoadDarkTheme()
	{
		Theme::Colored = ImVec4(0.200f, 0.100f, 0.270f, 1.000f);
		Theme::ColoredLight = ImVec4(0.264f, 0.185f, 0.345f, 1.000f);

		Theme::PropertyField = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		Theme::InfoField = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		Theme::BorderColored = ImVec4(0.23f, 0.04f, 0.36f, 0.75f);
		Theme::BorderColoredWeak = ImVec4(0.23f, 0.04f, 0.36f, 0.50f);
		Theme::ShadowColored = ImVec4(0.23f, 0.04f, 0.36f, 0.50f);

		
		auto& style = ImGui::GetStyle();
		auto& colors = ImGui::GetStyle().Colors;

		//================================//
		// Colors

		// Header
		colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Theme::Header);
		colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Theme::Header);
		colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Theme::Header);

		// Button
		colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame Background
		colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Theme::ControlField);
		colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Theme::ControlField);
		colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Theme::ControlField);

		// Tabs
		colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Theme::Titlebar);
		//colors[ImGuiCol_TabHovered] = ImColor(255, 255, 135, 30);
		//colors[ImGuiCol_TabActive] = ImColor(255, 255, 135, 60);
		colors[ImGuiCol_TabHovered] = ImColor(135, 100, 255, 30);
		colors[ImGuiCol_TabSelected] = ImColor(135, 100, 255, 60);
		colors[ImGuiCol_TabSelectedOverline] = ImColor(135, 100, 255, 120);
		colors[ImGuiCol_TabDimmed] = ImGui::ColorConvertU32ToFloat4(Theme::Titlebar);
		colors[ImGuiCol_TabDimmedSelected] = colors[ImGuiCol_TabHovered];
		colors[ImGuiCol_TabDimmedSelectedOverline] = colors[ImGuiCol_TabSelected];

		// Title Background
		colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Theme::Titlebar);
		colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Theme::Titlebar);
		colors[ImGuiCol_TitleBgCollapsed] = ImColor(0.15f, 0.1505f, 0.151f, 1.0f);

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImColor(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImColor(0.91f, 0.91f, 0.91f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImColor(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImColor(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImColor(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImColor(0.51f, 0.51f, 0.51f, 1.00f);

		// Checkmark
		colors[ImGuiCol_CheckMark] = ImColor(0.01f, 0.66f, 0.04f, 1.00f);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImColor(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImColor(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Theme::Text);

		// Separator
		colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Theme::BackgroundDark);
		colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Theme::Highlight);
		colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

		// Window Background
		colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Theme::Background);
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Theme::Background);
		colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Theme::BackgroundPopup);
		colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Theme::BackgroundDark);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Theme::Header);
		colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Theme::BackgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(Theme::Titlebar);

		// Nav
		colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(Theme::NavigationHighlight);

		//================================//
		// Styles

		style.WindowMinSize   = ImVec2(16.0f, 16.0f);
		style.IndentSpacing   = style.IndentSpacing * 0.5f;
		style.FrameBorderSize = 1.0f;
		style.FrameRounding = 2.5f;
	}

	void LoadLightTheme()
	{
		SK_NOT_IMPLEMENTED();
	}

	void LoadTheme(const std::filesystem::path& file)
	{
		SK_NOT_IMPLEMENTED();
	}

}

