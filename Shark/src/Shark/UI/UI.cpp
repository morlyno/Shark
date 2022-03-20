#include "skpch.h"
#include "UI.h"

#include "Shark/Utility/Math.h"
#include "Shark/Utility/String.h"

#include "Shark/Core/Application.h"

#include "Shark/Utility/Utility.h"
#include "Shark/Core/Buffer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <misc/cpp/imgui_stdlib.h>

namespace ImGui {

	static float CalcMaxPopupHeightFromItemCount(int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

	bool TableNextColumn(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
	{
		ImGuiContext& g = *GImGui;
		ImGuiTable* table = g.CurrentTable;
		if (!table)
			return false;

		if (table->IsInsideRow && table->CurrentColumn + 1 < table->ColumnsCount)
		{
			if (table->CurrentColumn != -1)
				TableEndCell(table);
			TableBeginCell(table, table->CurrentColumn + 1);
		}
		else
		{
			TableNextRow(row_flags, min_row_height);
			TableBeginCell(table, 0);
		}

		// Return whether the column is visible. User may choose to skip submitting items based on this return value,
		// however they shouldn't skip submitting for columns that may have the tallest contribution to row height.
		int column_n = table->CurrentColumn;
		return (table->RequestOutputMaskByIndex & ((ImU64)1 << column_n)) != 0;
	}

	bool BeginComboEx(ImGuiID id, const char* label, const char* preview_value, ImGuiComboFlags flags)
	{
		// Always consume the SetNextWindowSizeConstraint() call in our early return paths
		ImGuiContext& g = *GImGui;
		bool has_window_size_constraint = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0;
		g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

		const ImGuiStyle& style = g.Style;

		const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const float expected_w = CalcItemWidth();
		const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
		bool popup_open = IsPopupOpen(id, ImGuiPopupFlags_None);

		const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
		RenderNavHighlight(frame_bb, id);
		if (!(flags & ImGuiComboFlags_NoPreview))
			window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
		if (!(flags & ImGuiComboFlags_NoArrowButton))
		{
			ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
			ImU32 text_col = GetColorU32(ImGuiCol_Text);
			window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
			if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
				RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
		}
		RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
		if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
		{
			ImVec2 preview_pos = frame_bb.Min + style.FramePadding;
			if (g.LogEnabled)
				LogSetNextTextDecoration("{", "}");
			RenderTextClipped(preview_pos, ImVec2(value_x2, frame_bb.Max.y), preview_value, NULL, NULL, ImVec2(0.0f, 0.0f));
		}
		if (label_size.x > 0)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		if ((pressed || g.NavActivateId == id) && !popup_open)
		{
			if (window->DC.NavLayerCurrent == 0)
				window->NavLastIds[0] = id;
			OpenPopupEx(id, ImGuiPopupFlags_None);
			popup_open = true;
		}

		if (!popup_open)
			return false;

		if (has_window_size_constraint)
		{
			g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
			g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
		}
		else
		{
			if ((flags & ImGuiComboFlags_HeightMask_) == 0)
				flags |= ImGuiComboFlags_HeightRegular;
			IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));    // Only one
			int popup_max_height_in_items = -1;
			if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
			else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
			else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
			SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));
		}

		char name[16];
		ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

		// Position the window given a custom constraint (peak into expected window size so we can position it)
		// This might be easier to express with an hypothetical SetNextWindowPosConstraints() function.
		if (ImGuiWindow* popup_window = FindWindowByName(name))
			if (popup_window->WasActive)
			{
				// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
				ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
				if (flags & ImGuiComboFlags_PopupAlignLeft)
					popup_window->AutoPosLastDirection = ImGuiDir_Left; // "Below, Toward Left"
				else
					popup_window->AutoPosLastDirection = ImGuiDir_Down; // "Below, Toward Right (default)"
				ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
				ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
				SetNextWindowPos(pos);
			}

		// We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

		// Horizontally align ourselves with the framed text
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
		bool ret = Begin(name, NULL, window_flags);
		PopStyleVar();
		if (!ret)
		{
			EndPopup();
			IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
			return false;
		}
		return true;
	}

	template<typename T>
	bool CheckboxFlagsT(const char* label, T* flags, T flags_value)
	{
		bool all_on = (*flags & flags_value) == flags_value;
		bool any_on = (*flags & flags_value) != 0;
		bool pressed;
		if (!all_on && any_on)
		{
			ImGuiContext& g = *GImGui;
			ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
			g.CurrentItemFlags |= ImGuiItemFlags_MixedValue;
			pressed = Checkbox(label, &all_on);
			g.CurrentItemFlags = backup_item_flags;
		}
		else
		{
			pressed = Checkbox(label, &all_on);

		}
		if (pressed)
		{
			if (all_on)
				*flags |= flags_value;
			else
				*flags &= ~flags_value;
		}
		return pressed;
	}

	bool BeginCombo(const char* label, const char* preview_value_begin, const char* preview_value_end, ImGuiComboFlags flags)
	{
		// Always consume the SetNextWindowSizeConstraint() call in our early return paths
		ImGuiContext& g = *GImGui;
		bool has_window_size_constraint = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0;
		g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const float expected_w = CalcItemWidth();
		const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
		bool popup_open = IsPopupOpen(id, ImGuiPopupFlags_None);

		const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
		RenderNavHighlight(frame_bb, id);
		if (!(flags & ImGuiComboFlags_NoPreview))
			window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
		if (!(flags & ImGuiComboFlags_NoArrowButton))
		{
			ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
			ImU32 text_col = GetColorU32(ImGuiCol_Text);
			window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
			if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
				RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
		}
		RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
		if (preview_value_begin != NULL && !(flags & ImGuiComboFlags_NoPreview))
		{
			ImVec2 preview_pos = frame_bb.Min + style.FramePadding;
			if (g.LogEnabled)
				LogSetNextTextDecoration("{", "}");
			RenderTextClipped(preview_pos, ImVec2(value_x2, frame_bb.Max.y), preview_value_begin, preview_value_end, NULL, ImVec2(0.0f, 0.0f));
		}
		if (label_size.x > 0)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		if ((pressed || g.NavActivateId == id) && !popup_open)
		{
			if (window->DC.NavLayerCurrent == 0)
				window->NavLastIds[0] = id;
			OpenPopupEx(id, ImGuiPopupFlags_None);
			popup_open = true;
		}

		if (!popup_open)
			return false;

		if (has_window_size_constraint)
		{
			g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
			g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
		}
		else
		{
			if ((flags & ImGuiComboFlags_HeightMask_) == 0)
				flags |= ImGuiComboFlags_HeightRegular;
			IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));    // Only one
			int popup_max_height_in_items = -1;
			if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
			else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
			else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
			SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));
		}

		char name[16];
		ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

		// Position the window given a custom constraint (peak into expected window size so we can position it)
		// This might be easier to express with an hypothetical SetNextWindowPosConstraints() function.
		if (ImGuiWindow* popup_window = FindWindowByName(name))
			if (popup_window->WasActive)
			{
				// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
				ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
				if (flags & ImGuiComboFlags_PopupAlignLeft)
					popup_window->AutoPosLastDirection = ImGuiDir_Left; // "Below, Toward Left"
				else
					popup_window->AutoPosLastDirection = ImGuiDir_Down; // "Below, Toward Right (default)"
				ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
				ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
				SetNextWindowPos(pos);
			}

		// We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

		// Horizontally align ourselves with the framed text
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
		bool ret = Begin(name, NULL, window_flags);
		PopStyleVar();
		if (!ret)
		{
			EndPopup();
			IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
			return false;
		}
		return true;
	}

	bool Selectable(const char* label_begin, const char* label_end, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		// Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
		ImGuiID id = window->GetID(label_begin, label_end);
		ImVec2 label_size = CalcTextSize(label_begin, label_end, true);
		ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		ImVec2 pos = window->DC.CursorPos;
		pos.y += window->DC.CurrLineTextBaseOffset;
		ItemSize(size, 0.0f);

		// Fill horizontal space
		// We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
		const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
		const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
		const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
		if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
			size.x = ImMax(label_size.x, max_x - min_x);

		// Text stays at the submission position, but bounding box may be extended on both sides
		const ImVec2 text_min = pos;
		const ImVec2 text_max(min_x + size.x, pos.y + size.y);

		// Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
		ImRect bb(min_x, pos.y, text_max.x, text_max.y);
		if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
		{
			const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
			const float spacing_y = style.ItemSpacing.y;
			const float spacing_L = IM_FLOOR(spacing_x * 0.50f);
			const float spacing_U = IM_FLOOR(spacing_y * 0.50f);
			bb.Min.x -= spacing_L;
			bb.Min.y -= spacing_U;
			bb.Max.x += (spacing_x - spacing_L);
			bb.Max.y += (spacing_y - spacing_U);
		}
		//if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 0, 255)); }

		// Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackground for every Selectable..
		const float backup_clip_rect_min_x = window->ClipRect.Min.x;
		const float backup_clip_rect_max_x = window->ClipRect.Max.x;
		if (span_all_columns)
		{
			window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
			window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
		}

		bool item_add;
		if (flags & ImGuiSelectableFlags_Disabled)
		{
			ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
			g.CurrentItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
			item_add = ItemAdd(bb, id);
			g.CurrentItemFlags = backup_item_flags;
		}
		else
		{
			item_add = ItemAdd(bb, id);
		}

		if (span_all_columns)
		{
			window->ClipRect.Min.x = backup_clip_rect_min_x;
			window->ClipRect.Max.x = backup_clip_rect_max_x;
		}

		if (!item_add)
			return false;

		// FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
		// which would be advantageous since most selectable are not selected.
		if (span_all_columns && window->DC.CurrentColumns)
			PushColumnsBackground();
		else if (span_all_columns && g.CurrentTable)
			TablePushBackgroundChannel();

		// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
		ImGuiButtonFlags button_flags = 0;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
		if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
		if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
		if (flags & ImGuiSelectableFlags_Disabled) { button_flags |= ImGuiButtonFlags_Disabled; }
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
		if (flags & ImGuiSelectableFlags_AllowItemOverlap) { button_flags |= ImGuiButtonFlags_AllowItemOverlap; }

		if (flags & ImGuiSelectableFlags_Disabled)
			selected = false;

		const bool was_selected = selected;
		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

		// Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
		if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
		{
			if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
			{
				SetNavID(id, window->DC.NavLayerCurrent, window->DC.NavFocusScopeIdCurrent, ImRect(bb.Min - window->Pos, bb.Max - window->Pos));
				g.NavDisableHighlight = true;
			}
		}
		if (pressed)
			MarkItemEdited(id);

		if (flags & ImGuiSelectableFlags_AllowItemOverlap)
			SetItemAllowOverlap();

		// In this branch, Selectable() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		if (held && (flags & ImGuiSelectableFlags_DrawHoveredWhenHeld))
			hovered = true;
		if (hovered || selected)
		{
			const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
			RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
		}

		if (span_all_columns && window->DC.CurrentColumns)
			PopColumnsBackground();
		else if (span_all_columns && g.CurrentTable)
			TablePopBackgroundChannel();

		if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		RenderTextClipped(text_min, text_max, label_begin, label_end, &label_size, style.SelectableTextAlign, &bb);
		if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

		// Automatically close popups
		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.CurrentItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
			CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
		return pressed;
	}


}

namespace Shark::UI {

	namespace Utils {

		static void GridSeparator()
		{
			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;

			ImGuiTable* table = ImGui::GetCurrentTable();
			ImGuiTableColumn& collumn = table->Columns[table->CurrentColumn];
			float x1 = collumn.MinX;
			float x2 = collumn.MaxX;

			// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
			if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
				x1 += window->DC.Indent.x;

			// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
			float thickness_draw = 1.0f;
			float thickness_layout = 0.0f;

			const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
			window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));
		}

	}

	UIContext* GContext = nullptr;

	void SetBlend(bool blend)
	{
		ImGuiLayer& ctx = Application::Get().GetImGuiLayer();
		ctx.SubmitBlendCallback(blend);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Controls ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool BeginControls()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.Active = true;
			return true;
		}
		return false;
	}

	bool BeginControlsGrid(GridFlags flags)
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(!c.Active, "Controls Begin/End mismatch");

		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			c.ActiveGridFlags = flags;
			c.Active = true;
			return true;
		}
		return false;
	}

	void EndControls()
	{
		auto& c = GContext->Control;

		if (c.Active)
		{
			c.Active = false;
			c.WidgetCount = 0;
			c.ActiveGridFlags = GridFlag::None;
			ImGui::PopStyleVar(2);
			ImGui::EndTable();
		}
	}

	void EndControlsGrid()
	{
		return EndControls();
	}

	static void DrawControlSeperator(GridFlags grid)
	{
		if (grid == GridFlag::None)
			return;

		if (grid & GridFlag::Label)
		{
			ImGui::TableSetColumnIndex(0);
			UI::Utils::GridSeparator();
		}
		
		if (grid & GridFlag::Widget)
		{
			ImGui::TableSetColumnIndex(1);
			UI::Utils::GridSeparator();
		}

		ImGui::TableNextRow();
	}

	bool ControlBeginHelper(ImGuiID id)
	{
		auto& c = GContext->Control;

		if (!c.Active)
			return false;

		ImGui::PushID(id);
		ImGui::TableNextRow();

		if (c.WidgetCount++)
			DrawControlSeperator(c.ActiveGridFlags);

		return true;
	}

	bool ControlBeginHelper(std::string_view label)
	{
		auto& c = GContext->Control;

		if (!c.Active)
			return false;

		if (!label.empty())
			ImGui::PushID(label.data(), label.data() + label.size());
		ImGui::TableNextRow();

		if (c.WidgetCount++)
			DrawControlSeperator(c.ActiveGridFlags);

		return true;
	}

	void ControlEndHelper()
	{
		SK_CORE_ASSERT(ImGui::GetCurrentTable());
		ImGui::PopID();
	}

	template<typename T>
	static bool ControlDrag(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* speed, const T* min, const T* max, std::string_view fmt)
	{
		static_assert(std::is_scalar_v<T>);

		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

		bool changed = false;
		const bool HasFMT = !fmt.empty();
		if (!HasFMT)
			fmt = GContext->DefaultFormat[dataType];

		ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1.0f)) / comps - buttonSize;

		ImGui::PushItemWidth(width);

		size_t fmtOffset = 0;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val[0] = resetVal[0] ;
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::DragScalar("##X", dataType, val, (float)speed[0], min, max, fmt.data());

		if (components > 1)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = resetVal[1];
			};

			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##Y", dataType, &val[1], (float)speed[1], &min[1], &max[1], fmt.data() + fmtOffset);
		}

		if (components > 2)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = resetVal[2];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##Z", dataType, &val[2], (float)speed[2], &min[2], &max[2], fmt.data() + fmtOffset);
		}

		if (components > 3)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("W", { buttonSize, buttonSize }))
			{
				val[3] = resetVal[3];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::DragScalar("##W", dataType, &val[3], (float)speed[3], &min[3], &max[3], fmt.data() + fmtOffset);
		}

		ImGui::PopItemWidth();

		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlSlider(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* min, const T* max, std::string_view fmt)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);
		ImGui::TableSetColumnIndex(1);

		bool changed = false;
		const bool HasFMT = !fmt.empty();
		if (!HasFMT)
			fmt = GContext->DefaultFormat[dataType];

		ImGuiStyle& style = ImGui::GetStyle();
		const float comps = (float)components;
		const float buttonSize = ImGui::GetFrameHeight();
		const float widthAvail = ImGui::GetContentRegionAvail().x;
		const float width = (widthAvail - style.ItemSpacing.x * (comps - 1)) / comps - buttonSize;

		ImGui::PushItemWidth(width);

		size_t fmtOffset = 0;

		if (ImGui::Button("X", { buttonSize, buttonSize }))
		{
			val[0] = resetVal[0];
			changed = true;
		}
		ImGui::SameLine(0.0f, 0.0f);
		changed |= ImGui::SliderScalar("##X", dataType, &val[0], &min[0], &max[0], fmt.data());

		if (components > 1)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Y", { buttonSize, buttonSize }))
			{
				val[1] = resetVal[1];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##Y", dataType, &val[1], &min[1], &max[1], fmt.data() + fmtOffset);
		}

		if (components > 2)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("Z", { buttonSize, buttonSize }))
			{
				val[2] = resetVal[2];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##Z", dataType, &val[2], &min[2], &max[2], fmt.data() + fmtOffset);
		}

		if (components > 3)
		{
			if (HasFMT)
			{
				SK_CORE_ASSERT(fmtOffset != std::string::npos, "Invalid fmt");
				fmtOffset = fmt.find('\n', fmtOffset + 1);
			}

			ImGui::SameLine();
			if (ImGui::Button("W", { buttonSize, buttonSize }))
			{
				val[3] = resetVal[3];
				changed = true;
			}
			ImGui::SameLine(0.0f, 0.0f);
			changed |= ImGui::SliderScalar("##W", dataType, &val[3], &min[3], &max[3], fmt.data() + fmtOffset);
		}

		ImGui::PopItemWidth();
		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlScalar(std::string_view label, ImGuiDataType dataType, T* val, uint32_t components, const T* resetVal, const T* speed, const T* min, const T* max, std::string_view fmt, ControlType type)
	{
		if (type == ControlType::Slider)
			return ControlSlider(label, dataType, val, components, resetVal, min, max, fmt);

		if (type == ControlType::Drag)
			return ControlDrag(label, dataType, val, components, resetVal, speed, min, max, fmt);

		SK_CORE_ASSERT(false, "Unkown ControlType");
		return false;
	}

	bool Control(std::string_view label, float& val,     float resetVal,            float speed,            float min,            float max,            std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, Utility::ValuePtr(val), 1, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::vec2& val, const glm::vec2& resetVal, const glm::vec2& speed, const glm::vec2& min, const glm::vec2& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, Utility::ValuePtr(val), 2, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::vec3& val, const glm::vec3& resetVal, const glm::vec3& speed, const glm::vec3& min, const glm::vec3& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, Utility::ValuePtr(val), 3, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::vec4& val, const glm::vec4& resetVal, const glm::vec4& speed, const glm::vec4& min, const glm::vec4& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_Float, Utility::ValuePtr(val), 4, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }

	bool Control(std::string_view label, int& val,        int resetVal,               int speed,               int min,               int max,               std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, Utility::ValuePtr(val), 1, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec2& val, const glm::ivec2& resetVal, const glm::ivec2& speed, const glm::ivec2& min, const glm::ivec2& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, Utility::ValuePtr(val), 2, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec3& val, const glm::ivec3& resetVal, const glm::ivec3& speed, const glm::ivec3& min, const glm::ivec3& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, Utility::ValuePtr(val), 3, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::ivec4& val, const glm::ivec4& resetVal, const glm::ivec4& speed, const glm::ivec4& min, const glm::ivec4& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_S32, Utility::ValuePtr(val), 4, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }

	bool Control(std::string_view label, uint32_t& val,   uint32_t resetVal,          uint32_t speed,          uint32_t min,          uint32_t max,          std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, Utility::ValuePtr(val), 1, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec2& val, const glm::uvec2& resetVal, const glm::uvec2& speed, const glm::uvec2& min, const glm::uvec2& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, Utility::ValuePtr(val), 2, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec3& val, const glm::uvec3& resetVal, const glm::uvec3& speed, const glm::uvec3& min, const glm::uvec3& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, Utility::ValuePtr(val), 3, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	bool Control(std::string_view label, glm::uvec4& val, const glm::uvec4& resetVal, const glm::uvec4& speed, const glm::uvec4& min, const glm::uvec4& max, std::string_view fmt, ControlType type) { return ControlScalar(label, ImGuiDataType_U32, Utility::ValuePtr(val), 4, Utility::ValuePtr(resetVal), Utility::ValuePtr(speed), Utility::ValuePtr(min), Utility::ValuePtr(max), fmt, type); }
	
	bool ControlAngle(std::string_view label, float& radians,     float resetVal,            float speed,            float min,            float max,            std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, speed, min, max, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec2& radians, const glm::vec2& resetVal, const glm::vec2& speed, const glm::vec2& min, const glm::vec2& max, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, speed, min, max, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec3& radians, const glm::vec3& resetVal, const glm::vec3& speed, const glm::vec3& min, const glm::vec3& max, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, speed, min, max, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }
	bool ControlAngle(std::string_view label, glm::vec4& radians, const glm::vec4& resetVal, const glm::vec4& speed, const glm::vec4& min, const glm::vec4& max, std::string_view fmt, ControlType type) { auto degrees = glm::degrees(radians); const bool changed = Control(label, degrees, resetVal, speed, min, max, fmt, type); if (changed) { radians = glm::radians(degrees); return true; } return false; }

	bool ControlColor(std::string_view label, glm::vec4& color)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		const bool changed = ImGui::ColorEdit4("##ColorEdit4", glm::value_ptr(color));

		ControlEndHelper();
		return changed;
	}

	bool Control(std::string_view label, bool& val)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		const bool changed = ImGui::Checkbox("##Checkbox", &val);

		ControlEndHelper();
		return changed;
	}

	template<typename T>
	static bool ControlFlagsT(std::string_view label, T& val, const T& flag)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		const bool changed = ImGui::CheckboxFlagsT("##label", &val, flag);

		ControlEndHelper();
		return changed;
	}

	bool ControlFlags(std::string_view label,  int16_t& val,  int16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label,  int32_t& val,  int32_t flag) { return ControlFlagsT(label, val, flag); }
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag) { return ControlFlagsT(label, val, flag); }

	template<typename T>
	bool ControlComboT(std::string_view label, T& index, const std::string_view items[], uint32_t itemsCount)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);

		bool changed = false;
		std::string_view preview = items[index];
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("#combo", preview.data(), preview.data() + preview.size()))
		{
			for (T i = 0; i < (T)itemsCount; i++)
			{
				std::string_view currentItem = items[i];
				if (ImGui::Selectable(currentItem.data(), currentItem.data() + currentItem.size(), i == index))
				{
					index = i;
					changed = true;
				}
			}

			ImGui::EndCombo();
		}

		ControlEndHelper();
		return changed;
	}

	bool Control(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool Control(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	bool Control(std::string_view label, int& index, const std::string_view items[], uint32_t itemsCount)
	{
		return ControlComboT(label, index, items, itemsCount);
	}

	void Control(std::string_view label, std::string_view str, TextFlags flags, TextFlags labelFlags)
	{
		if (!ControlBeginHelper(label))
			return;

		ImGui::TableSetColumnIndex(0);
		Text(label, labelFlags | PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		Text(str, flags | PrivateTextFlag::StringDefault);

		ControlEndHelper();
	}

	void Control(std::string_view label, std::string&& str, TextFlags flags, TextFlags labelFlags)
	{
		Control(label, std::string_view(str), flags, labelFlags);
	}

	void Control(std::string_view label, const char* str, TextFlags flags, TextFlags labelFlags)
	{
		Control(label, std::string_view(str), flags, labelFlags);
	}

	void Control(std::string_view label, const std::filesystem::path& filePath, TextFlags flags, TextFlags labelFalgs)
	{
		Control(label, filePath.string(), flags, labelFalgs);
	}

	void Control(std::string_view label, const std::string& str, TextFlags flags, TextFlags labelFalgs)
	{
		Control(label, std::string_view(str), flags, labelFalgs);
	}

	bool ControlCustomBegin(std::string_view label, TextFlags labelFlags)
	{
		if (!ControlBeginHelper(label))
			return false;

		ImGui::TableSetColumnIndex(0);
		Text(label, labelFlags | PrivateTextFlag::LabelDefault);

		ImGui::TableSetColumnIndex(1);
		return true;
	}

	void ControlCustomEnd()
	{
		ControlEndHelper();
	}

	void Control(ImGuiID id, void(*func)(void* data, ImGuiID id), void* data)
	{
		if (!ControlBeginHelper(id))
			return;

		func(data, id);

		ControlEndHelper();
	}

	void Control(std::string_view label, void(*func)(void* data, ImGuiID id), void* data)
	{
		const ImGuiID id = ImGui::GetID(label.data(), label.data() + label.size());
		if (!ControlBeginHelper(id))
			return;

		ImGui::TableSetColumnIndex(0);
		Text(label);

		ImGui::TableSetColumnIndex(1);
		func(data, id);

		ControlEndHelper();
	}


	void Text(std::string_view str, TextFlags flags)
	{
		UI::ScopedStyle s;
		if (flags & TextFlag::Disabled)
			s.Push(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

		if (flags & TextFlag::Selectable)
			return TextSelectable(str);

		if (flags & TextFlag::Aligned)
			ImGui::AlignTextToFramePadding();

		ImGui::TextEx(str.data(), str.data() + str.size());
	}

	void Text(std::string&& str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const char* str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void Text(const std::filesystem::path& filePath, TextFlags flags)
	{
		std::string str = filePath.string();
		Text(std::string_view(str), flags);
	}

	void Text(const std::string& str, TextFlags flags)
	{
		Text(std::string_view(str), flags);
	}

	void TextSelectable(std::string_view str)
	{
		//Buffer buffer;
		//buffer.Allocate(str.size());
		//buffer.Write(str.data(), str.size());
		//ImGui::InputText("##InputText", buffer.As<char>(), buffer.Size, ImGuiInputTextFlags_ReadOnly);
		//buffer.Release();

		const auto& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, style.FramePadding.y));

		const ImVec2 textSize = ImGui::CalcTextSize(str.data(), str.data() + str.size());
		const ImVec2 itemSize = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), textSize.x + style.FramePadding.x * 2.0f + 1.0f, textSize.y + style.FramePadding.y * 2.0f);

		ImGui::InputTextEx("##InputText", nullptr, (char*)str.data(), (int)str.size(), itemSize, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	UIContext::UIContext()
	{
		DefaultFormat[ImGuiDataType_S8] = "%d";
		DefaultFormat[ImGuiDataType_U8] = "%u";
		DefaultFormat[ImGuiDataType_S16] = "%d";
		DefaultFormat[ImGuiDataType_U16] = "%u";
		DefaultFormat[ImGuiDataType_S32] = "%d";
		DefaultFormat[ImGuiDataType_U32] = "%u";
		DefaultFormat[ImGuiDataType_S64] = "%d";
		DefaultFormat[ImGuiDataType_U64] = "%u";
		DefaultFormat[ImGuiDataType_Float] = "%.2f";
		DefaultFormat[ImGuiDataType_Double] = "%.2f";
	}

	UIContext* CreateContext()
	{
		UIContext* ctx = new UIContext();
		if (!GContext)
			SetContext(ctx);
		return ctx;
	}

	void DestroyContext(UIContext* ctx)
	{
		if (ctx == nullptr)
			ctx = GContext;
		if (ctx == GContext)
			SetContext(nullptr);
		delete ctx;
	}

	void SetContext(UIContext* ctx)
	{
		GContext = ctx;
	}

	UIContext* GetContext()
	{
		return GContext;
	}

	void NewFrame()
	{
		auto& c = GContext->Control;
		SK_CORE_ASSERT(GContext->Control.Active == false, "Controls Begin/End mismatch");
		SK_CORE_ASSERT(GContext->Control.WidgetCount == 0);
		SK_CORE_ASSERT(GContext->Control.ActiveGridFlags == GridFlag::None);
	}

}
