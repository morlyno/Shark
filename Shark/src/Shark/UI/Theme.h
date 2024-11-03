#pragma once

#include <imgui.h>
#include <filesystem>

namespace Shark::UI::Colors {

	void LoadDarkTheme();
	void LoadLightTheme();
	void LoadTheme(const std::filesystem::path& file);

	namespace Theme {

		constexpr ImU32 ButtonDark = IM_COL32(0x18, 0x18, 0x18, 0xFF);
		constexpr ImU32 ButtonHoveredDark = IM_COL32(0x21, 0x21, 0x21, 0xFF);
		constexpr ImU32 ButtonActiveDark = IM_COL32(0x1B, 0x1B, 0x1B, 0xFF);

		inline ImVec4 Colored;
		inline ImVec4 ColoredLight;

		// Content Browser
		inline ImVec4 PropertyField;
		inline ImVec4 InfoField;
		inline ImVec4 BorderColored;
		inline ImVec4 BorderColoredWeak;
		inline ImVec4 ShadowColored;

		// Console
		constexpr ImU32 LogTrace = IM_COL32(0x3E, 0x83, 0xB3, 0xFF);
		constexpr ImU32 LogInfo = IM_COL32(0x33, 0xB3, 0x1A, 0xFF);
		constexpr ImU32 LogWarn = IM_COL32(0xD9, 0xB1, 0x00, 0xFF);
		constexpr ImU32 LogError = IM_COL32(0xCC, 0x33, 0x1A, 0xFF);
		constexpr ImU32 LogCritical = IM_COL32(0xF2, 0x26, 0x1A, 0xFF);
		constexpr ImU32 LogTimeColor = IM_COL32(0x92, 0x92, 0x92, 0xFF);


		//constexpr ImU32 Highlight = IM_COL32(39, 185, 242, 255);
		constexpr ImU32 Highlight = IM_COL32(167, 167, 167, 255);
		constexpr ImU32 Background = IM_COL32(36, 36, 36, 255);
		constexpr ImU32 BackgroundDark = IM_COL32(26, 26, 26, 255);
		constexpr ImU32 BackgroundPopup = IM_COL32(50, 50, 50, 255);
		constexpr ImU32 Titlebar = IM_COL32(21, 21, 21, 255);
		constexpr ImU32 Text = IM_COL32(192, 192, 192, 255);
		constexpr ImU32 TextBrighter = IM_COL32(210, 210, 210, 255);
		constexpr ImU32 TextDarker = IM_COL32(128, 128, 128, 255);
		constexpr ImU32 TextError = IM_COL32(230, 51, 51, 255);
		constexpr ImU32 Header = IM_COL32(47, 47, 47, 255);
		constexpr ImU32 ControlField = IM_COL32(15, 15, 15, 255);
		constexpr ImU32 Selection = IM_COL32(58, 48, 91, 255);
		constexpr ImU32 SelectionMuted = IM_COL32(88, 78, 121, 120);
		constexpr ImU32 SelectionCompliment = IM_COL32(91, 48, 58, 255);
		constexpr ImU32 NavigationHighlight = IM_COL32(75, 62, 118, 255);
		constexpr ImU32 Blue = IM_COL32(73, 177, 255, 255);
		constexpr ImU32 NiceBlue = IM_COL32(51, 161, 230, 255);
		//constexpr ImU32 Selection = IM_COL32(48, 40, 76, 255);
	};

	inline ImU32 WithHue(const ImColor& color, float hue)
	{
		const ImVec4& colRow = color.Value;
		float h, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, sat, val);
		return ImColor::HSV(hue, sat, val);
	}

	inline ImU32 WithSaturation(const ImColor& color, float saturation)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, saturation, val);
	}

	inline ImU32 WithValue(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, value);
	}


	inline ImU32 WithAddedHue(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(std::clamp(hue + value, 0.0f, 1.0f), sat, val);
	}

	inline ImU32 WithAddedSaturation(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::clamp(sat + value, 0.0f, 1.0f), val);
	}

	inline ImU32 WithAddedValue(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::clamp(val + value, 0.0f, 1.0f));
	}


	inline ImU32 WithMultipliedHue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
	}

	inline ImU32 WithMultipliedSaturation(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
	}

	inline ImU32 WithMultipliedValue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
	}

}
