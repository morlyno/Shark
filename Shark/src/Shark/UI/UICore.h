#pragma once

#include "Shark/Core/Base.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/UIUtilities.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {
	class Texture2D;
	class Image2D;
	class ImageView;
	class Project;
	class AssetManager;
}

#define MAX_INPUT_BUFFER_LENGTH 256

#define UI_INPUT_TEXT_FILTER(_filterItems) [](ImGuiInputTextCallbackData* data) -> int { constexpr std::string_view view = _filterItems; return view.find(data->EventChar) != std::string_view::npos; }

namespace Shark::UI {

	enum
	{
		DefaultHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_Framed,
		DefaultThinHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Helpers ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ImGuiID GetCurrentID();
	const char* GenerateID();
	const char* GenerateID(const char* label);
	ImGuiID GenerateUniqueID();

	void PushID();
	void PopID();

	inline void ShiftCursor(const ImVec2& delta) { ImGui::SetCursorPos(ImGui::GetCursorPos() + delta); }
	inline void ShiftCursor(float deltaX, float deltaY) { ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(deltaX, deltaY)); }
	inline void ShiftCursorX(float deltaX) { ShiftCursor({ deltaX, 0.0f }); }
	inline void ShiftCursorY(float deltaY) { ShiftCursor({ 0.0f, deltaY }); }

	inline void SetCursorScreenPosX(float x) { ImGui::SetCursorScreenPos(ImVec2(x, ImGui::GetCursorScreenPos().y)); }
	inline void SetCursorScreenPosY(float y) { ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, y)); }

	void SetInputEnabled(bool enabled);
	bool IsInputEnabled();

	bool BeginMenubar(const ImRect& rect);
	void EndMenubar();

	bool BeginControlsGrid();
	void EndControlsGrid();

	struct BeginTreeNodeSettings { bool OpenByDefault = true; bool Bold = false; };
	bool BeginTreeNode(const char* name, const BeginTreeNodeSettings& settings = {});
	void EndTreeNode();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Widgets ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool MultiSelectInvisibleButton(const char* str_id, bool selected, ImVec2 size, ImGuiButtonFlags flags = 0);
	bool MultiSelectInvisibleButton(ImGuiID id, bool selected, ImVec2 size, ImGuiButtonFlags flags = 0);

	void DrawBackground(ImRect rect, ImU32 color, float rounding = 0.0f, ImDrawFlags drawFlags = 0);
	void DrawBorder(ImRect rect, ImU32 color, float rounding, ImDrawFlags drawFlags = 0);
	void DrawItemFrame(ImRect rect, ImU32 bgColor = Colors::Theme::ControlField, ImU32 borderColor = Colors::Theme::BackgroundDark, float rounding = GImGui->Style.FrameRounding, ImDrawFlags drawFlags = 0);
	void DrawItemFrame(ImVec2 size, ImU32 bgColor = Colors::Theme::ControlField, ImU32 borderColor = Colors::Theme::BackgroundDark, float rounding = GImGui->Style.FrameRounding, ImDrawFlags drawFlags = 0);

	void DrawButton(std::string_view text, ImVec2 textAlign, ImU32 colorNormal, ImU32 colorHoverd, ImU32 colorPressed, ImRect rect);
	void DrawButton(std::string_view text, ImVec2 textAlign, ImRect rect);
	void DrawButton(std::string_view text, ImRect rect);

	void DrawTextAligned(std::string_view text, ImVec2 align, ImRect rect);
	void DrawTextAligned(std::string_view text, ImVec2 align, ImVec2 size);

	void DrawButtonFrame(ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);
	void DrawButtonFrame(ImVec2 min, ImVec2 max, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);
	void DrawButtonFrame(ImVec2 min, ImVec2 max);
	void DrawButtonFrame();

	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);
	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);

	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);
	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);

	void DrawImageButton(Ref<Image2D> imageNormal, Ref<Image2D> imageHovered, Ref<Image2D> imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);
	void DrawImageButton(Ref<Image2D> image, ImU32 tintNormal = 0xFFFFFFFF, ImU32 tintHovered = 0xFFFFFFFF, ImU32 tintPressed = 0xFFFFFFFF);
	void DrawImageButton(Ref<Texture2D> textureNormal, Ref<Texture2D> textureHovered, Ref<Texture2D> texturePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);
	void DrawImageButton(Ref<Texture2D> texture, ImU32 tintNormal = 0xFFFFFFFF, ImU32 tintHovered = 0xFFFFFFFF, ImU32 tintPressed = 0xFFFFFFFF);

	void DrawImage(Ref<Image2D> image, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	void DrawImage(Ref<ImageView> image, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	void DrawImage(Ref<Texture2D> texture, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	struct ImageArgs
	{
		ImVec2 UV0 = { 0, 0 };
		ImVec2 UV1 = { 1, 1 };
		ImColor TintColor = { 1, 1, 1, 1 };
		ImColor BackgroundColor = { 0, 0, 0, 0 };
	};

	void Image(Ref<Texture2D> texture, const ImVec2& size, const ImageArgs args);
	void Image(Ref<Image2D> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	void Image(Ref<ImageView> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	void Image(Ref<Texture2D> texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool TextureEdit(const char* textID, Ref<Texture2D>& texture, const ImVec2& size, bool clearButton, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec4& tint_col);

	void Text(const char* fontName, const char* text);

	template<typename TEnum>
	bool EnumCombo(const char* label, TEnum& selected)
	{
		auto preview = magic_enum::enum_name(selected);
		bool modified = false;
		if (UI::BeginCombo(label, preview.data()))
		{
			constexpr auto options = magic_enum::enum_entries<TEnum>();
			for (auto option : options)
			{
				const bool isSelected = option.first == selected;
				if (ImGui::Selectable(option.second.data(), isSelected))
				{
					selected = option.first;
					modified = true;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			UI::EndCombo();
		}
		return modified;
	}

}
