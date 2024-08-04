#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/ImGui/ImGuiFonts.h"
#include "Shark/UI/Controls.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/glm.hpp>

#ifdef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec4 operator*(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs); }
static inline ImVec4 operator/(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs); }
#endif

template<typename Char>
struct fmt::formatter<ImVec2, Char> : fmt::formatter<float, Char>
{
	template<typename FormatContext>
	auto format(const ImVec2& vec2, FormatContext& ctx) const -> decltype(ctx.out())
	{
		auto&& out = ctx.out();
		format_to(out, "[");

		fmt::formatter<float, Char>::format(vec2.x, ctx);
		format_to(out, ", ");
		fmt::formatter<float, Char>::format(vec2.y, ctx);

		format_to(out, "]");

		return out;
	}
};

namespace Shark {
	class ImGuiLayer;
	class Texture2D;
	class Image2D;
	class ImageView;
	class Project;
	class AssetManager;
}

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

	inline ImGuiID GetID(int intID)                                            { return GImGui->CurrentWindow->GetID(intID); }
	inline ImGuiID GetID(void* ptrID)                                          { return GImGui->CurrentWindow->GetID(ptrID); }
	inline ImGuiID GetID(const char* strID, const char* strEnd = nullptr)      { return GImGui->CurrentWindow->GetID(strID, strEnd); }
	inline ImGuiID GetID(const std::string& strID)                             { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(const std::string_view& strID)                        { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(UUID uuid)                                            { static_assert(sizeof(void*) == sizeof(uint64_t)); return GImGui->CurrentWindow->GetID((void*)uuid.Value());}

	inline ImGuiID GetIDWithSeed(int intID, uint32_t seed)                     { const ImGuiID id = ImHashData(&intID, sizeof(int), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(void* ptrID, uint32_t seed)                   { const ImGuiID id = ImHashData(&ptrID, sizeof(void*), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(const char* strID, uint32_t seed)             { const ImGuiID id = ImHashStr(strID, 0, seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(const std::string& strID, uint32_t seed)      { const ImGuiID id = ImHashStr(strID.c_str(), strID.size(), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(std::string_view strID, uint32_t seed)        { const ImGuiID id = ImHashStr(strID.data(), strID.size(), seed); ImGui::KeepAliveID(id); return id; }

	inline void PushID(ImGuiID id)                                             { GImGui->CurrentWindow->IDStack.push_back(id) ;}
	inline void PushID(UUID id)                                                { PushID(GetID(id)); }
	inline void PushID(int intID)                                              { PushID(GetID(intID)); }
	inline void PushID(void* ptrID)                                            { PushID(GetID(ptrID)); }
	inline void PushID(const char* strID, const char* strEnd = nullptr)        { PushID(GetID(strID, strEnd)); }
	inline void PushID(const std::string& strID)                               { PushID(GetID(strID)); }
	inline void PushID(const std::string_view& strID)                          { PushID(GetID(strID)); }
	inline void PopID()                                                        { GImGui->CurrentWindow->IDStack.pop_back(); }

	inline ImGuiID GetCurrentID()                                              { return GImGui->CurrentWindow->IDStack.back(); }

	ImGuiID GenerateID();

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

	ImU32 ToColor32(const ImVec4& color);
	ImU32 ToColor32(const ImVec4& color, float alpha);

	ImVec4 GetColor(ImGuiCol color, float override_alpha);

	void PushFramedTextAlign(const ImVec2& align);
	void PopFramedTextAlign();

	ImVec2 CalcItemSizeFromText(const char* text, const char* textEnd = nullptr);

 	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Widgets ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DrawBackground(ImRect rect, ImU32 color, float rounding = 0.0f, ImDrawFlags drawFlags = 0);
	void DrawBorder(ImRect rect, ImU32 color, float rounding, ImDrawFlags drawFlags = 0);

	void DrawButton(std::string_view text, ImVec2 textAlign, ImU32 colorNormal, ImU32 colorHoverd, ImU32 colorPressed, ImRect rect);
	void DrawButton(std::string_view text, ImVec2 textAlign, ImRect rect);
	void DrawButton(std::string_view text, ImRect rect);

	void DrawTextAligned(std::string_view text, ImVec2 align, ImRect rect);

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

	void Image(Ref<Image2D> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(Ref<ImageView> image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Texture(Ref<Texture2D> texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	bool TextureEdit(const char* textID, Ref<Texture2D>& texture, const ImVec2& size, bool clearButton, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* strID, Ref<Texture2D> texture, const ImVec2& image_size, const ImVec4& tint_col);


	bool BeginMenubar(const ImRect& rect);
	void EndMenubar();


	void Text(const char* str);
	void Text(std::string_view str);
	void Text(const std::string& string);
	void Text(const std::filesystem::path& path);

	template<typename... TArgs>
	void Text(std::string_view fmt, TArgs&&... args)
	{
		Text(fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...));
	}

	void TextFramed(std::string_view fmt, ...);
	bool Search(ImGuiID id, char* buffer, int bufferSize);
	
	bool InputFileName(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput);
	bool InputPath(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput);
	bool InputPath(const char* label, char* buffer, int bufferSize);

	template<typename T>
	bool SliderScalar(const char* label, ImGuiDataType data_type, T& data, uint32_t min, uint32_t max, const char* format = NULL, ImGuiSliderFlags flags = 0)
	{
		return ImGui::SliderScalar(label, data_type, &data, &min, &max, format, flags);
	}

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
		ScopedColorConditional(ImGuiCol color, const ImColor& val, bool push) { if (push) { m_Pushed = true; ImGui::PushStyleColor(color, val.Value); } }
		~ScopedColorConditional() { if (m_Pushed) ImGui::PopStyleColor(); }
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

		void Push(ImGuiStyleVar style, const ImVec2& val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
		}

		void Push(ImGuiStyleVar style, float val)
		{
			ImGui::PushStyleVar(style, val);
			m_Count++;
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
			PopAll();
		}

		void Push(ImGuiCol color, const ImColor& val)
		{
			ImGui::PushStyleColor(color, val.Value);
			m_Count++;
		}

		void Pop()
		{
			ImGui::PopStyleColor();
			m_Count--;
		}

		void PopAll()
		{
			ImGui::PopStyleColor(m_Count);
			m_Count = 0;
		}

	private:
		uint32_t m_Count = 0;
	};

	struct LagacyScopedStyleStack
	{
		LagacyScopedStyleStack() = default;
		LagacyScopedStyleStack(ImGuiStyleVar idx, float val)          { Push(idx, val); }
		LagacyScopedStyleStack(ImGuiStyleVar idx, const ImVec2& val)  { Push(idx, val); }
		LagacyScopedStyleStack(ImGuiCol idx, const ImVec4& col)       { Push(idx, col); }
		~LagacyScopedStyleStack()                                     { Pop(); }

		void Pop()                                         { ImGui::PopStyleVar(VarCount); ImGui::PopStyleColor(ColorCount); VarCount = 0; ColorCount = 0; }
		void PopVar(int count = -1)                        { ImGui::PopStyleVar(count); count == -1 ? VarCount = 0 : VarCount -= count; }
		void PopColor(int count = -1)                      { ImGui::PopStyleColor(count); count == -1 ? ColorCount = 0 : ColorCount -= count; }

		void Push(ImGuiStyleVar idx, float val)            { ImGui::PushStyleVar(idx, val); VarCount++; }
		void Push(ImGuiStyleVar idx, const ImVec2& val)    { ImGui::PushStyleVar(idx, val); VarCount++; }
		void Push(ImGuiCol idx, const ImVec4& col)         { ImGui::PushStyleColor(idx, col); ColorCount++; }

		uint32_t VarCount = 0;
		uint32_t ColorCount = 0;
	};

	struct ScopedIndent
	{
		ScopedIndent(float indent = 0) : Indent(indent) { ImGui::Indent(indent); }
		~ScopedIndent() { ImGui::Unindent(Indent); }

		float Indent;
	};

	struct ScopedClipRect
	{
		ScopedClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false)
		{
			ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
		}
		ScopedClipRect(const ImRect& clip_rect, bool intersect_with_current_clip_rect = false)
		{
			ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, intersect_with_current_clip_rect);
		}
		~ScopedClipRect()
		{
			ImGui::PopClipRect();
		}
	};

	struct ScopedItemFlag
	{
		ScopedItemFlag(ImGuiItemFlags flag, bool enabled)
		{
			ImGui::PushItemFlag(flag, enabled);
		}
		~ScopedItemFlag()
		{
			ImGui::PopItemFlag();
		}
	};

	struct ScopedDisabled
	{
		ScopedDisabled(bool disable)
		{
			ImGui::BeginDisabled(disable);
		}
		~ScopedDisabled()
		{
			ImGui::EndDisabled();
		}
	};

	struct ScopedFont
	{
		ScopedFont(const char* name)
		{
			Fonts::Push(name);
		}
		~ScopedFont()
		{
			Fonts::Pop();
		}
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
		ImVec2 FramedTextAlign = ImVec2(0.0f, 0.0f);
		std::stack<ImVec2> FramedTextAlignStack;

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
