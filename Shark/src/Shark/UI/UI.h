#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/ImGui/ImGuiFonts.h"
#include "Shark/UI/Controls.h"
#include "Shark/ImGui/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {
	class ImGuiLayer;
	class Texture2D;
	class Image2D;
	class ImageView;
	class Project;
	class AssetManager;
}

#define MAX_INPUT_BUFFER_LENGTH 260

namespace Shark::UI {

	struct UIControl;
	struct UIContext;

	enum
	{
		DefaultHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_Framed,
		DefaultThinHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected
	};

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Helpers ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline ImGuiID GetID(int intID)                                            { return ImGui::GetID(intID); }
	inline ImGuiID GetID(void* ptrID)                                          { return ImGui::GetID(ptrID); }
	inline ImGuiID GetID(const char* strID, const char* strEnd = nullptr)      { return ImGui::GetID(strID, strEnd); }
	inline ImGuiID GetID(const std::string& strID)                             { return ImGui::GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(const std::string_view& strID)                        { return ImGui::GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(UUID uuid)                                            { static_assert(sizeof(void*) == sizeof(uint64_t)); return ImGui::GetID((void*)uuid.Value());}

	ImGuiID GetIDWithSeed(UUID id, uint32_t seed);
	ImGuiID GetIDWithSeed(int intID, uint32_t seed);
	ImGuiID GetIDWithSeed(void* ptrID, uint32_t seed);
	ImGuiID GetIDWithSeed(const char* strID, uint32_t seed);
	ImGuiID GetIDWithSeed(const std::string& strID, uint32_t seed);
	ImGuiID GetIDWithSeed(std::string_view strID, uint32_t seed);

	void PushID(ImGuiID id);
	inline void PushID(UUID id)                                                { PushID(GetID(id)); }
	inline void PushID(int intID)                                              { PushID(GetID(intID)); }
	inline void PushID(void* ptrID)                                            { PushID(GetID(ptrID)); }
	inline void PushID(const char* strID, const char* strEnd = nullptr)        { PushID(GetID(strID, strEnd)); }
	inline void PushID(const std::string& strID)                               { PushID(GetID(strID)); }
	inline void PushID(const std::string_view& strID)                          { PushID(GetID(strID)); }
	void PopID();

	ImGuiID GetCurrentID();
	ImGuiID GenerateID();
	ImGuiID GenerateUniqueID();

	inline void ShiftCursor(const ImVec2& delta)                                { ImGui::SetCursorPos(ImGui::GetCursorPos() + delta); }
	inline void ShiftCursorX(float deltaX)                                      { ShiftCursor({ deltaX, 0.0f }); }
	inline void ShiftCursorY(float deltaY)                                      { ShiftCursor({ 0.0f, deltaY }); }

	inline void SetCursorScreenPosX(float x)                                    { ImGui::SetCursorScreenPos(ImVec2(x, ImGui::GetCursorScreenPos().y)); }
	inline void SetCursorScreenPosY(float y)                                    { ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, y)); }

	ImRect GetItemRect();
	ImRect RectExpand(const ImRect& rect, float x, float y);
	ImRect RectExpand(const ImRect& rect, const ImVec2& xy);
	ImRect RectOffset(const ImRect& rect, float x, float y);
	ImRect RectOffset(const ImRect& rect, const ImVec2& offset);

	bool HighlightNav(ImGuiID id, ImGuiNavHighlightFlags flags);

	ImVec2 CalcImageSizeByWidth(Ref<Image2D> image, float width);
	ImVec2 CalcImageSizeByHeight(Ref<Image2D> image, float height);

 	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Widgets ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool MultiSelectInvisibleButton(const char* str_id, bool selected, ImVec2 size, ImGuiButtonFlags flags = 0);
	bool MultiSelectInvisibleButton(ImGuiID id, bool selected, ImVec2 size, ImGuiButtonFlags flags = 0);

	void DrawBackground(ImRect rect, ImU32 color, float rounding = 0.0f, ImDrawFlags drawFlags = 0);
	void DrawBorder(ImRect rect, ImU32 color, float rounding, ImDrawFlags drawFlags = 0);

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

	void DrawImage(Ref<Image2D> image, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void DrawImage(Ref<ImageView> image, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void DrawImage(Ref<Texture2D> texture, const ImRect& rect, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	void Image(Ref<Image2D> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(Ref<ImageView> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Texture(Ref<Texture2D> texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	bool TextureEdit(const char* textID, Ref<Texture2D>& texture, const ImVec2& size, bool clearButton, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec4& tint_col);

	void Text(const char* fontName, const char* text);

	bool BeginMenubar(const ImRect& rect);
	void EndMenubar();

	void TextFramed(std::string_view fmt, ...);
	bool Search(ImGuiID id, char* buffer, int bufferSize);
	
	bool InputFileName(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput);
	bool InputFileName(const char* label, char* buffer, int bufferSize);
	bool InputPath(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput);
	bool InputPath(const char* label, char* buffer, int bufferSize);

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Types //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct ScopedID
	{
		ScopedID(ImGuiID id)                               { PushID(id); }
		ScopedID(UUID id)                                  { PushID(id); }
		ScopedID(int intID)                                { PushID(intID); }
		ScopedID(void* ptrID)                              { PushID(ptrID); }
		ScopedID(const char* strID, const char* strEnd)    { PushID(strID, strEnd); }
		ScopedID(const std::string& strID)                 { PushID(strID); }
		ScopedID(const std::string_view& strID)            { PushID(strID); }
		~ScopedID()                                        { PopID(); }
	};

	struct ScopedStyle
	{
		ScopedStyle(ImGuiStyleVar style, float val)              { ImGui::PushStyleVar(style, val); }
		ScopedStyle(ImGuiStyleVar style, const ImVec2& val)      { ImGui::PushStyleVar(style, val); }
		~ScopedStyle()                                           { ImGui::PopStyleVar(); }
	};
	
	struct ScopedColor
	{
		ScopedColor(ImGuiCol color, const ImVec4& val)           { ImGui::PushStyleColor(color, val); }
		ScopedColor(ImGuiCol color, ImU32 val)                   { ImGui::PushStyleColor(color, val); }
		~ScopedColor()                                           { ImGui::PopStyleColor(); }
	};

	class ScopedColorConditional
	{
	public:
		ScopedColorConditional(ImGuiCol color, const ImColor& val, bool push)
		{
			if (push)
			{
				m_Pushed = true;
				ImGui::PushStyleColor(color, val.Value);
			}
		}
		~ScopedColorConditional()
		{
			if (m_Pushed)
				ImGui::PopStyleColor();
		}
	private:
		bool m_Pushed = false;
	};

	class ScopedStyleStack
	{
	public:
		ScopedStyleStack() = default;

		ScopedStyleStack(ImGuiStyleVar style, const ImVec2& val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		ScopedStyleStack(ImGuiStyleVar style, const float& val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedStyleStack(ImGuiStyleVar style, const ImVec2& val, TArgs&&... args)
			: ScopedStyleStack(args...)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedStyleStack(ImGuiStyleVar style, const float& val, TArgs&&... args)
			: ScopedStyleStack(args...)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		~ScopedStyleStack()
		{
			ImGui::PopStyleVar(m_Count);
		}

	private:
		uint32_t m_Count = 0;
	};

	class ScopedColorStack
	{
	public:
		ScopedColorStack() = default;

		ScopedColorStack(ImGuiCol color, const ImColor& val)
		{
			ImGui::PushStyleColor(color, val.Value);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedColorStack(ImGuiCol color, const ImColor& val, TArgs&&... args)
			: ScopedColorStack(args...)
		{
			ImGui::PushStyleColor(color, val.Value);
			m_Count++;
		}

		~ScopedColorStack()
		{
			ImGui::PopStyleColor(m_Count);
		}

	private:
		uint32_t m_Count = 0;
	};
	
	struct ScopedItemFlag
	{
		ScopedItemFlag(ImGuiItemFlags flag, bool enabled) { ImGui::PushItemFlag(flag, enabled); }
		~ScopedItemFlag()                                 { ImGui::PopItemFlag(); }
	};

	struct ScopedDisabled
	{
		ScopedDisabled(bool disable)                      { ImGui::BeginDisabled(disable); }
		~ScopedDisabled()                                 { ImGui::EndDisabled(); }
	};

	struct ScopedFont
	{
		ScopedFont(const char* name)                      { Fonts::Push(name); }
		~ScopedFont()                                     { Fonts::Pop(); }
	};
	
	struct ScopedClipRect
	{
		ScopedClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false) { ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect); }
		ScopedClipRect(const ImRect& clip_rect, bool intersect_with_current_clip_rect = false)                                  { ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, intersect_with_current_clip_rect); }
		~ScopedClipRect()                                                                                                       { ImGui::PopClipRect(); }
	};

	struct ScopedIndent
	{
		ScopedIndent(float indent = 0) : Indent(indent)   { ImGui::Indent(indent); }
		~ScopedIndent()                                   { ImGui::Unindent(Indent); }

		float Indent;
	};

	struct UIControl
	{
		bool Active = false;
		uint32_t WidgetCount = 0;
		bool DrawSeparator = false;
	};

	struct UIContext
	{
		ImGuiLayer* ImGuiLayer;
		UIControl Control;

		UIContext();
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Init // Update /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	UIContext* CreateContext(ImGuiLayer* imguiLayer);
	void DestroyContext(UIContext* ctx = nullptr);

	void SetContext(UIContext* ctx);
	UIContext* GetContext();

	void NewFrame();

}
