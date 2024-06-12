#include "skpch.h"
#include "UI.h"

#include "Shark/Core/Application.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/File/FileSystem.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/Icons.h"

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

	}

	UIContext* GContext = nullptr;

	ImGuiID GenerateID()
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		return window->GetID(window->IDStack.front());
	}

	ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	ImRect RectExpand(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	ImRect RectExpand(const ImRect& rect, const ImVec2& xy)
	{
		return RectExpand(rect, xy.x, xy.y);
	}

	ImRect RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	ImRect RectOffset(const ImRect& rect, const ImVec2& offset)
	{
		return RectOffset(rect, offset.x, offset.y);
	}

	ImU32 ToColor32(const ImVec4& color)
	{
		ImU32 out;
		out = ((ImU32)IM_F32_TO_INT8_SAT(color.x)) << IM_COL32_R_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.y)) << IM_COL32_G_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.z)) << IM_COL32_B_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.w)) << IM_COL32_A_SHIFT;
		return out;
	}

	ImU32 ToColor32(const ImVec4& color, float alpha)
	{
		ImU32 out;
		out = ((ImU32)IM_F32_TO_INT8_SAT(color.x)) << IM_COL32_R_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.y)) << IM_COL32_G_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(color.z)) << IM_COL32_B_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(alpha)) << IM_COL32_A_SHIFT;
		return out;
	}

	ImVec4 GetColor(ImGuiCol color, float override_alpha)
	{
		const ImVec4& col = ImGui::GetStyleColorVec4(color);
		return {
			col.x,
			col.y,
			col.z,
			override_alpha
		};
	}

	void PushFramedTextAlign(const ImVec2& align)
	{
		GContext->FramedTextAlignStack.push(GContext->FramedTextAlign);
		GContext->FramedTextAlign = align;
	}

	void PopFramedTextAlign()
	{
		GContext->FramedTextAlign = GContext->FramedTextAlignStack.top();
		GContext->FramedTextAlignStack.pop();
	}

	ImVec2 CalcItemSizeFromText(const char* text, const char* textEnd)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 textSize = ImGui::CalcTextSize(text, textEnd);
		return { textSize.x + style.FramePadding.x * 2.0f, textSize.y + style.FramePadding.y * 2.0f };
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Controls ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Text(std::string_view str)
	{
		ImGui::TextEx(str.data(), str.data() + str.size());
	}

	void Text(const char* str)
	{
		return Text(std::string_view(str));
	}

	void Text(const std::string& string)
	{
		return Text(std::string_view(string));
	}

	void Text(const std::filesystem::path& path)
	{
		return Text(path.string());
	}

	void TextFramed(std::string_view fmt, ...)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		const ImGuiStyle style = ImGui::GetStyle();
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() };
		
		if (GImGui->NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth)
			size.x = GImGui->NextItemData.Width;

		const ImRect textRect = { pos + style.FramePadding, pos + size - style.FramePadding };
		const ImRect frameRect = { pos, pos + size };

		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, 0))
			return;

		va_list args;
		va_start(args, fmt);
		const char* text, *text_end;
		ImFormatStringToTempBufferV(&text, &text_end, fmt.data(), args);
		va_end(args);

		const ImU32 frameColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
		ImGui::RenderFrame(frameRect.Min, frameRect.Max, frameColor, true, style.FrameRounding);
		ImGui::RenderTextClipped(textRect.Min, textRect.Max, text, text_end, nullptr, GContext->FramedTextAlign, &textRect);
	}

	bool Search(ImGuiID id, char* buffer, int bufferSize)
	{
		ScopedID scopedID(id);
		ImGui::SetNextItemAllowOverlap();
		const bool changed = ImGui::InputText("##search", buffer, bufferSize);

		// "\xef\x80\x82 Search ..."

		const auto& style = ImGui::GetStyle();

		if (!buffer[0])
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImFont* textFont = ImGui::GetFont();

			const float fontSize = ImGui::GetFontSize();
			const ImVec2 textPos = ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0);
			const ImVec2 iconMin = textPos + ImVec2(1, 1);
			const ImVec2 iconMax = textPos + ImVec2(fontSize - 2, fontSize - 2);

			drawList->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

			drawList->AddImage(Icons::Search->GetViewID(), iconMin, iconMax, ImVec2(1, 0), ImVec2(0, 1), UI::Colors::Theme::TextDarker);
			drawList->AddText(textPos + ImVec2(fontSize, 0.0f), UI::Colors::Theme::TextDarker, "Search ...");

			drawList->PopClipRect();
		}

		bool clear = false;

#if 0
		{
			const float fontSize = ImGui::GetFontSize();
			const ImVec2 buttonSize = { fontSize + style.FramePadding.x * 2.0f, fontSize + style.FramePadding.y * 2.0f };

			ImGui::SameLine(0, 0);
			MoveCursorX(-buttonSize.x);

			clear = ImGui::InvisibleButton("clear_button", buttonSize);

			UI::DrawImageButton(Icons::ClearIcon,
								UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Text, 0.9f),
								UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::Text, 1.2f),
								UI::Colors::Theme::TextDarker,
								UI::RectExpand(UI::GetItemRect(), -style.FramePadding));
		}
#endif

		if (clear)
		{
			memset(buffer, '\0', bufferSize);
			ImGui::SetKeyboardFocusHere(-1);
		}

		return changed || clear;
	}

	bool InputFileName(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput)
	{
		auto filter = [](ImGuiInputTextCallbackData* data) -> int
		{
			if (FileSystem::InvalidCharactersW.find(data->EventChar) != std::wstring_view::npos)
			{
				*(bool*)data->UserData = true;
				return 1;
			}
			return 0;
		};

		return ImGui::InputText(label, buffer, bufferSize, ImGuiInputTextFlags_CallbackCharFilter, filter, &out_InvalidInput);
	}

	bool InputPath(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput)
	{
		auto filter = [](ImGuiInputTextCallbackData* data) -> int
		{
			if (!(data->EventChar == L'\\' || data->EventChar == L'/') && FileSystem::InvalidCharactersW.find(data->EventChar) != std::wstring_view::npos)
			{
				*(bool*)data->UserData = true;
				return 1;
			}
			return 0;
		};

		out_InvalidInput = false;
		return ImGui::InputText(label, buffer, bufferSize, ImGuiInputTextFlags_CallbackCharFilter, filter, &out_InvalidInput);
	}

	bool InputPath(const char* label, char* buffer, int bufferSize)
	{
		bool dummy;
		return InputPath(label, buffer, bufferSize, dummy);
	}

	UIContext::UIContext()
	{
	}

	UIContext* CreateContext(ImGuiLayer* imguiLayer)
	{
		UIContext* ctx = sknew UIContext();
		ctx->ImGuiLayer = imguiLayer;
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
		skdelete ctx;
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
		SK_CORE_ASSERT(GContext->Control.DrawSeparator == false);
		SK_CORE_ASSERT(GContext->FramedTextAlignStack.size() == 0);
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
			GContext->ImGuiLayer->AddImage(imagePressed);
			drawList->AddImage(imagePressed->GetViewID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		}
		else if (ImGui::IsItemHovered())
		{
			GContext->ImGuiLayer->AddImage(imageHovered);
			drawList->AddImage(imageHovered->GetViewID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		}
		else
		{
			GContext->ImGuiLayer->AddImage(imageNormal);
			drawList->AddImage(imageNormal->GetViewID(), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
		}

		GContext->ImGuiLayer->BindFontSampler();
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

	void Image(Ref<ImageView> image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		GContext->ImGuiLayer->AddImage(image->GetImage());
		ImGui::Image(image->GetViewID(), size, uv0, uv1, tint_col, border_col);
		GContext->ImGuiLayer->BindFontSampler();
	}

	void Image(Ref<Image2D> image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		GContext->ImGuiLayer->AddImage(image);
		ImGui::Image(image->GetViewID(), size, uv0, uv1, tint_col, border_col);
		GContext->ImGuiLayer->BindFontSampler();
	}

	void Texture(Ref<Texture2D> texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		GContext->ImGuiLayer->AddImage(texture->GetImage());
		ImGui::Image(texture->GetViewID(), size, uv0, uv1, tint_col, border_col);
		GContext->ImGuiLayer->BindFontSampler();
	}

	bool TextureEdit(const char* textID, Ref<Texture2D>& texture, const ImVec2& size, bool clearButton, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		bool changed = false;
		Ref<Texture2D> displayTexture = texture;

		Texture(displayTexture, size, uv0, uv1, tint_col, border_col);
		utils::DragDropTargetAsset<Texture2D>(ImGuiDragDropFlags_AcceptNoDrawDefaultRect, [&texture, &changed](Ref<Texture2D> dragDropTexture)
		{
			texture = dragDropTexture;
			changed = true;
		});

		if (clearButton)
		{
			const ImVec2 cursorPosition = ImGui::GetCursorPos();
			ImGui::SameLine(0.0f, 0.0f);

			const auto& style = ImGui::GetStyle();
			UI::MoveCursorX(-(ImGui::GetFontSize() + style.FramePadding.x * 0.5f));

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
		GContext->ImGuiLayer->AddImage(texture->GetImage());
		const bool pressed = ImGui::ImageButton(strID, texture->GetViewID(), image_size, uv0, uv1, bg_col, tint_col);
		GContext->ImGuiLayer->BindFontSampler();
		return pressed;
	}

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec4& tint_col)
	{
		GContext->ImGuiLayer->AddImage(texture->GetImage());
		const bool pressed = ImGui::ImageButton(strID, texture->GetViewID(), image_size, { 0, 0 }, { 1, 1 }, { 0, 0, 0, 0 }, tint_col);
		GContext->ImGuiLayer->BindFontSampler();
		return pressed;
	}

	bool BeginMenubar(const ImRect& barRectangle)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		/*if (!(window->Flags & ImGuiWindowFlags_MenuBar))
			return false;*/

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
		ImRect bar_rect = UI::RectOffset(barRectangle, 0.0f, padding.y);// window->MenuBarRect();
		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
						 IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(bar_rect.Max.y + window->Pos.y));

		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
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
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavDisableMouseHover = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		//IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

}
