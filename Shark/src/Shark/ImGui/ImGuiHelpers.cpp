#include "skpch.h"
#include "ImGuiHelpers.h"

#include <imgui_internal.h>
#include <vadefs.h>

namespace ImGui {

	void ReadOnlyCheckbox(const char* label, bool value)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label, NULL);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id))
		{
			IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
			return;
		}

		//bool hovered, held;
		//ButtonBehavior(total_bb, id, &hovered, &held);

		const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
		RenderNavHighlight(total_bb, id);
		RenderFrame(check_bb.Min, check_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
		ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
		bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
		if (mixed_value)
		{
			// Undocumented tristate/mixed/indeterminate checkbox (#2644)
			// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
			ImVec2 pad(ImMax(1.0f, ImFloor(square_sz / 3.6f)), ImMax(1.0f, ImFloor(square_sz / 3.6f)));
			window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
		}
		else if (value)
		{
			const float pad = ImMax(1.0f, ImFloor(square_sz / 6.0f));
			RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
		}

		ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
		if (g.LogEnabled)
			LogRenderedText(&label_pos, mixed_value ? "[~]" : value ? "[x]" : "[ ]");
		if (label_size.x > 0.0f)
			RenderText(label_pos, label, NULL);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	}

}
