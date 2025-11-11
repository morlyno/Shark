#include "skpch.h"
#include "UICore.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Hash.h"
#include "Shark/Asset/AssetManager.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui {

	static float CalcMaxPopupHeightFromItemCount(int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

}

namespace Shark::UI {

	namespace utils {

		template<typename TAsset, typename TFunc>
		static void DragDropTargetAsset(ImGuiDragDropFlags flags, const TFunc& func)
		{
			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Asset", flags);
				if (payload)
				{
					AssetHandle handle = *(AssetHandle*)payload->Data;
					func(AssetManager::GetAsset<TAsset>(handle));
				}
				ImGui::EndDragDropTarget();
			}
		}

		static ImTextureID GetTextureID(Ref<ViewableResource> viewable)
		{
			return reinterpret_cast<ImTextureID>(viewable.Detach());
		}

	}

	static int s_PushIDContext = 0;
	static uint64_t s_GenerateIDIndex = 0;

	static uint32_t s_IDCounter = 0;
	static char s_IDBuffer[2 + 16 + 1];

	static char s_LabelIDBuffer[255];

	ImGuiID GetCurrentID()
	{
		return GImGui->CurrentWindow->IDStack.back();
	}

	const char* GenerateID()
	{
		fmt::format_to_n(s_IDBuffer, std::size(s_IDBuffer), "##{}\0", s_IDCounter++);
		return s_IDBuffer;
	}

	const char* GenerateID(const char* label)
	{
		fmt::format_to_n(s_LabelIDBuffer, std::size(s_LabelIDBuffer), "{}##{}\0", label, s_IDCounter++);
		return s_LabelIDBuffer;
	}

	ImGuiID GenerateUniqueID()
	{
		return (ImGuiID)Hash::GenerateFNV(s_GenerateIDIndex++);
	}

	void PushID()
	{
		ImGui::PushID(s_PushIDContext++);
		s_IDCounter = 0;
	}

	void PopID()
	{
		ImGui::PopID();
		s_PushIDContext--;
	}

	void SetInputEnabled(bool enabled)
	{
		auto& io = ImGui::GetIO();
		if (enabled)
		{
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			io.ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
			io.ConfigNavCaptureKeyboard = true;
		}
		else
		{
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
			io.ConfigNavCaptureKeyboard = false;
		}
	}

	bool IsInputEnabled()
	{
		const auto& io = ImGui::GetIO();
		return io.ConfigNavCaptureKeyboard && (io.ConfigFlags & ImGuiConfigFlags_NoMouse) == 0 && (io.ConfigFlags & ImGuiConfigFlags_NoKeyboard) == 0;
	}

	bool BeginMenubar(const ImRect& barRectangle)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		//if (!(window->Flags & ImGuiWindowFlags_MenuBar))
		//	return false;

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##MenuBar");

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
		const float border_top = ImMax(IM_ROUND(window->WindowBorderSize * 0.5f - window->TitleBarHeight), 0.0f);
		const float border_half = IM_ROUND(window->WindowBorderSize * 0.5f);
		ImRect bar_rect = UI::RectOffset(barRectangle, 0.0f, window->WindowPadding.y);// window->MenuBarRect();
		ImRect clip_rect(ImFloor(bar_rect.Min.x + border_half), ImFloor(bar_rect.Min.y + border_top), ImFloor(ImMax(bar_rect.Min.x, bar_rect.Max.x - ImMax(window->WindowRounding, border_half))), ImFloor(bar_rect.Max.y));
		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->DC.MenuBarOffset.x, bar_rect.Min.y + window->DC.MenuBarOffset.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;
		ImGui::AlignTextToFramePadding();
		return true;
	}

	void EndMenubar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		//IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
		IM_ASSERT(window->DC.MenuBarAppending);

		// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
				const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check (FIXME: Seems unnecessary)
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				// FIXME-NAV: How to deal with this when not using g.IO.ConfigNavCursorVisibleAuto?
				if (g.NavCursorVisible)
				{
					g.NavCursorVisible = false; // Hide nav cursor for the current frame so we don't see the intermediary selection. Will be set again
					g.NavCursorHideFrames = 2;
				}
				g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		ImGui::PopClipRect();
		ImGui::PopID();
		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.

		// FIXME: Extremely confusing, cleanup by (a) working on WorkRect stack system (b) not using a Group confusingly here.
		ImGuiGroupData& group_data = g.GroupStack.back();
		group_data.EmitItem = false;
		ImVec2 restore_cursor_max_pos = group_data.BackupCursorMaxPos;
		window->DC.IdealMaxPos.x = ImMax(window->DC.IdealMaxPos.x, window->DC.CursorMaxPos.x - window->Scroll.x); // Convert ideal extents for scrolling layer equivalent.
		ImGui::EndGroup(); // Restore position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
		window->DC.CursorMaxPos = restore_cursor_max_pos;
	}

	bool BeginControlsGrid()
	{
		if (ImGui::BeginTable("ControlsTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			//ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing * 0.5f);

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.25f);
			ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch, 0.75f);
			PushID();
			return true;
		}
		return false;
	}

	void EndControlsGrid()
	{
		if (ImGui::GetCurrentTable())
		{
			PopID();
			ImGui::PopStyleVar(1);
			ImGui::EndTable();
		}
	}

	bool BeginTreeNode(const char* name, const BeginTreeNodeSettings& settings)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (settings.OpenByDefault)
			treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		UI::ScopedFont font("Bold", settings.Bold);

		return ImGui::TreeNodeEx(name, treeFlags);
	}

	void EndTreeNode()
	{
		ImGui::TreePop();
	}

	bool MultiSelectInvisibleButton(const char* str_id, bool selected, ImVec2 size_arg, ImGuiButtonFlags flags)
	{
		return MultiSelectInvisibleButton(ImGui::GetID(str_id), selected, size_arg, flags);
	}

	bool MultiSelectInvisibleButton(ImGuiID id, bool selected, ImVec2 size_arg, ImGuiButtonFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		// Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
		IM_ASSERT(size_arg.x != 0.0f && size_arg.y != 0.0f);

		ImVec2 size = ImGui::CalcItemSize(size_arg, 0.0f, 0.0f);
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
		ImGui::ItemSize(size);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		const bool is_multi_select = (g.LastItemData.ItemFlags & ImGuiItemFlags_IsMultiSelect) != 0;

		if (is_multi_select)
			ImGui::MultiSelectItemHeader(id, &selected, &flags);

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

		if (is_multi_select)
			ImGui::MultiSelectItemFooter(id, &selected, &pressed);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, "", g.LastItemData.StatusFlags);
		return pressed;
	}

	void DrawBackground(ImRect rect, ImU32 color, float rounding, ImDrawFlags drawFlags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		window->DrawList->AddRectFilled(rect.Min, rect.Max, color, rounding, drawFlags);
	}

	void DrawBorder(ImRect rect, ImU32 color, float rounding, ImDrawFlags drawFlags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		const float border_size = g.Style.FrameBorderSize;
		if (border_size > 0.0f)
		{
			//window->DrawList->AddRect(rect.Min + ImVec2(1, 1), rect.Max + ImVec2(1, 1), shadowColor, rounding, 0, border_size);
			window->DrawList->AddRect(rect.Min, rect.Max, color, rounding, drawFlags, border_size);
		}
	}

	void DrawItemFrame(ImRect rect, ImU32 bgColor, ImU32 borderColor, float rounding, ImDrawFlags drawFlags)
	{
		DrawBackground(rect, bgColor, rounding, drawFlags);
		DrawBorder(rect, borderColor, rounding, drawFlags);
	}

	void DrawItemFrame(ImVec2 size, ImU32 bgColor, ImU32 borderColor, float rounding, ImDrawFlags drawFlags)
	{
		DrawItemFrame(RectFromSize(ImGui::GetCursorScreenPos(), size), bgColor, borderColor, rounding, drawFlags);
	}

	void DrawButton(std::string_view text, ImVec2 textAlign, ImU32 colorNormal, ImU32 colorHoverd, ImU32 colorPressed, ImRect rect)
	{
		const auto& style = ImGui::GetStyle();
		if (ImGui::IsItemActive())
			DrawBackground(rect, colorPressed, style.FrameRounding);
		else if (ImGui::IsItemHovered())
			DrawBackground(rect, colorHoverd, style.FrameRounding);
		else
			DrawBackground(rect, colorNormal, style.FrameRounding);

		DrawBorder(rect, UI::Colors::Theme::BackgroundDark, style.FrameRounding);
		ImGui::RenderTextClipped(rect.Min + style.FramePadding, rect.Max - style.FramePadding, text.data(), text.data() + text.length(), nullptr, textAlign);
	}

	void DrawButton(std::string_view text, ImVec2 textAlign, ImRect rect)
	{
		DrawButton(text, textAlign, ImGui::GetColorU32(ImGuiCol_Button), ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetColorU32(ImGuiCol_ButtonActive), rect);
	}

	void DrawButton(std::string_view text, ImRect rect)
	{
		DrawButton(text, ImVec2(0.0f, 0.5f), rect);
	}

	void DrawTextAligned(std::string_view text, ImVec2 align, ImRect rect)
	{
		ImGui::RenderTextClipped(rect.Min, rect.Max, text.data(), text.data() + text.length(), nullptr, align);
	}

	void DrawTextAligned(std::string_view text, ImVec2 align, ImVec2 size)
	{
		ImRect rect;
		rect.Min = ImGui::GetCursorScreenPos();
		rect.Max = rect.Min + size;
		DrawTextAligned(text, align, rect);
	}

	void DrawButtonFrame(ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawButtonFrame(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), tintNormal, tintHovered, tintPressed);
	}

	void DrawButtonFrame(ImVec2 min, ImVec2 max, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		const auto& style = ImGui::GetStyle();
		if (ImGui::IsItemActive())
			ImGui::RenderFrame(min, max, tintNormal, true, style.FrameRounding);
		else if (ImGui::IsItemHovered())
			ImGui::RenderFrame(min, max, tintHovered, true, style.FrameRounding);
		else
			ImGui::RenderFrame(min, max, tintPressed, true, style.FrameRounding);
	}

	void DrawButtonFrame(ImVec2 min, ImVec2 max)
	{
		const auto& style = ImGui::GetStyle();
		if (ImGui::IsItemActive())
			ImGui::RenderFrame(min, max, ImGui::GetColorU32(ImGuiCol_ButtonActive), true, style.FrameRounding);
		else if (ImGui::IsItemHovered())
			ImGui::RenderFrame(min, max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), true, style.FrameRounding);
		else
			ImGui::RenderFrame(min, max, ImGui::GetColorU32(ImGuiCol_Button), true, style.FrameRounding);
	}

	void DrawButtonFrame()
	{
		DrawButtonFrame(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive())
		{
			drawList->AddImage(utils::GetTextureID(imagePressed.Detach()), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		}
		else if (ImGui::IsItemHovered())
		{
			drawList->AddImage(utils::GetTextureID(imageHovered.Detach()), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		}
		else
		{
			drawList->AddImage(utils::GetTextureID(imageNormal.Detach()), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
		}
	}

	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawImageButton(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	}

	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawImageButton(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	}

	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawImageButton(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	}

	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawImageButton(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawImageButton(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawImageButton(textureNormal->GetImage(), textureHovered->GetImage(), texturePressed->GetImage(), tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	}

	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawImageButton(textureNormal->GetImage(), textureHovered->GetImage(), texturePressed->GetImage(), tintNormal, tintHovered, tintPressed, rectangle);
	}

	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		DrawImageButton(texture->GetImage(), tintNormal, tintHovered, tintPressed, rectMin, rectMax);
	}

	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawImageButton(texture->GetImage(), tintNormal, tintHovered, tintPressed, rectangle);
	}

	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawImageButton(textureNormal->GetImage(), textureHovered->GetImage(), texturePressed->GetImage(), tintNormal, tintHovered, tintPressed);
	}

	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		DrawImageButton(texture->GetImage(), tintNormal, tintHovered, tintPressed);
	}

	void DrawImage(Ref<Image2D> image, const ImRect& rect, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddImage(utils::GetTextureID(image.Detach()), rect.Min, rect.Max, uv0, uv1, ImGui::GetColorU32(tint_col));
	}

	void DrawImage(Ref<ImageView> image, const ImRect& rect, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		window->DrawList->AddImage(utils::GetTextureID(image.Detach()), rect.Min, rect.Max, uv0, uv1, ImGui::GetColorU32(tint_col));
	}

	void DrawImage(Ref<Texture2D> texture, const ImRect& rect, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		DrawImage(texture->GetImage(), rect, uv0, uv1, tint_col);
	}

	void Image(Ref<Texture2D> texture, const ImVec2& size, const ImageArgs args)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGui::ImageWithBg(utils::GetTextureID(texture.Detach()), size, args.UV0, args.UV1, args.BackgroundColor, args.TintColor);
	}

	void Image(Ref<ImageView> imageView, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGui::ImageWithBg(utils::GetTextureID(imageView), size, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
	}

	void Image(Ref<Image2D> image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGui::ImageWithBg(utils::GetTextureID(image), size, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
	}

	void Image(Ref<Texture2D> texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGui::ImageWithBg(utils::GetTextureID(texture), size, uv0, uv1, ImVec4(0, 0, 0, 0), tint_col);
	}

	bool TextureEdit(const char* textID, Ref<Texture2D>& texture, const ImVec2& size, bool clearButton, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col)
	{
		bool changed = false;
		Ref<Texture2D> displayTexture = texture;

		UI::Image(displayTexture, size, uv0, uv1, tint_col);
		utils::DragDropTargetAsset<Texture2D>(ImGuiDragDropFlags_AcceptNoDrawDefaultRect, [&texture, &changed](Ref<Texture2D> droppedTexture)
		{
			texture = droppedTexture;
			changed = true;
		});

		if (ImGui::BeginItemTooltip())
		{
			const auto& spec = displayTexture->GetSpecification();
			float width = (float)std::max<uint32_t>(std::min<uint32_t>(512, spec.Width), 128);
			float height = width * displayTexture->GetVerticalAspectRatio();

			if (height > 512)
			{
				height = 512;
				width = height * displayTexture->GetAspectRatio();
			}

			UI::Image(displayTexture, { width, height });
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(displayTexture);
			ImGui::Text(fmt::format("File: {}", metadata.FilePath.generic_string()));
			ImGui::Text(fmt::format("Format: {}", spec.Format));
			ImGui::Text(fmt::format("Size: [w={}, h={}] Mips: {}", spec.Width, spec.Height, displayTexture->GetImage()->GetSpecification().MipLevels));
			ImGui::Text(fmt::format("Filter: {}", spec.Filter));
			ImGui::Text(fmt::format("Address: {}", spec.Address));
			ImGui::Text(fmt::format("MaxAnisotropy: {}", spec.MaxAnisotropy));
			ImGui::EndTooltip();
		}

		if (clearButton)
		{
			const ImVec2 cursorPosition = ImGui::GetCursorPos();
			ImGui::SameLine(0.0f, 0.0f);

			const auto& style = ImGui::GetStyle();
			UI::ShiftCursorX(-(ImGui::GetFontSize() + style.FramePadding.x * 0.5f));

			UI::ScopedID buttonID(textID, nullptr);
			const float buttonSize = ImGui::GetFontSize();
			if (ImGui::Button("X"))
			{
				texture = nullptr;
				changed = true;
			}

			ImGui::SetCursorPos(cursorPosition);
		}

		return changed;
	}

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImGui::ImageButton(strID, utils::GetTextureID(texture), image_size, uv0, uv1, bg_col, tint_col);
	}

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec4& tint_col)
	{
		return ImGui::ImageButton(strID, utils::GetTextureID(texture), image_size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, tint_col);
	}

	void Text(const char* fontName, const char* text)
	{
		UI::ScopedFont font(fontName);
		ImGui::Text(text);
	}

}
