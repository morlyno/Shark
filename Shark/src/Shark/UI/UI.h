#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Asset/Asset.h"

#include "Shark/ImGui/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>
#include <stack>

#define UI_DRAGDROP_DIRECTORY_TYPE "DIRECTORY"

#ifdef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec4 operator*(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs); }
static inline ImVec4 operator/(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs); }
#endif

inline std::ostream& operator<<(std::ostream& out, const ImVec2& v)
{
	return out << fmt::format("[{}, {}]", v.x, v.y);
}

template<typename Char>
struct fmt::formatter<ImVec2, Char> : fmt::formatter<float, Char>
{
	template<typename FormatContext>
	auto format(const ImVec2& vec2, FormatContext& ctx) -> decltype(ctx.out())
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

namespace Shark::UI {

	struct UIControl;
	struct UIContext;

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Flags //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum
	{
		DefaultTreeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected,
		TreeNodeSeperatorFlags = DefaultTreeNodeFlags,

		DefaultHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_Framed,
		DefaultThinHeaderFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected
	};

	namespace TextFlag {
		enum TextFlag : uint16_t
		{
			None = 0,
			Aligned = BIT(0),
			Selectable = BIT(1),
			Disabled = BIT(2)
		};
	}
	using TextFlags = std::underlying_type_t<TextFlag::TextFlag>;

	namespace PrivateTextFlag {
		enum PrivateTextFlag : uint16_t
		{
			LabelDefault = TextFlag::Aligned,
			StringDefault = TextFlag::Aligned
		};
	}

	namespace GridFlag {
		enum GridFlag : uint16_t
		{
			None = 0,
			Label = BIT(0),
			Widget = BIT(1),

			Full = Label | Widget
		};
	}
	using GridFlags = std::underlying_type_t<GridFlag::GridFlag>;

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Helpers ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline ImGuiID GetID(int intID)                                            { return GImGui->CurrentWindow->GetID(intID); }
	inline ImGuiID GetID(void* ptrID)                                          { return GImGui->CurrentWindow->GetID(ptrID); }
	inline ImGuiID GetID(const char* strID, const char* strEnd = nullptr)      { return GImGui->CurrentWindow->GetID(strID, strEnd); }
	inline ImGuiID GetID(const std::string& strID)                             { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(const std::string_view& strID)                        { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(UUID uuid)                                            { static_assert(sizeof(void*) == sizeof(uint64_t)); return GImGui->CurrentWindow->GetID((void*)(uint64_t)uuid);}

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

	inline void MoveCursor(const ImVec2& delta)                                { ImGui::SetCursorPos(ImGui::GetCursorPos() + delta); }
	inline void MoveCursorX(float deltaX)                                      { MoveCursor({ deltaX, 0.0f }); }
	inline void MoveCursorY(float deltaY)                                      { MoveCursor({ 0.0f, deltaY }); }

	ImU32 ToColor32(const ImVec4& color);
	ImU32 ToColor32(const ImVec4& color, float alpha);

	ImVec4 GetColor(ImGuiCol color, float override_alpha);

	void PushFramedTextAlign(const ImVec2& align);
	void PopFramedTextAlign();

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Controls ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct DragDropID
	{
		static constexpr const char* Asset = "ASSET";
		static constexpr const char* Entity = "ENTITY_ID";
		static constexpr const char* Directroy = "DIRECTORY";
	};

	bool BeginControls();
	bool BeginControlsGrid(GridFlags flags = GridFlag::Label);
	bool BeginControls(ImGuiID syncID);
	bool BeginControlsGrid(ImGuiID syncID, GridFlags flags = GridFlag::Label);
	void EndControls();
	void EndControlsGrid();

	void ControlSetupColumns(std::string_view label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f);

	bool ControlBeginHelper(ImGuiID id);
	bool ControlBeginHelper(std::string_view strID);
	void ControlEndHelper();

	float ControlContentRegionWidth();

	bool Header(std::string_view label, ImGuiTreeNodeFlags flags = DefaultHeaderFlags);
	void PopHeader();

	bool Control(std::string_view label, float& val, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, double& val, float speed = 0.1f, double min = 0.0, double max = 0.0, const char* fmt = nullptr);

	bool Control(std::string_view label, int8_t& val, float speed = 0.1f, int8_t min = 0, int8_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int16_t& val, float speed = 0.1f, int16_t min = 0, int16_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int32_t& val, float speed = 0.1f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int64_t& val, float speed = 0.1f, int64_t min = 0, int64_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, uint8_t& val, float speed = 0.1f, uint8_t min = 0, uint8_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, uint16_t& val, float speed = 0.1f, uint16_t min = 0, uint16_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, uint32_t& val, float speed = 0.1f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, uint64_t& val, float speed = 0.1f, uint64_t min = 0, uint64_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::vec2& val, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::vec3& val, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::vec4& val, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::ivec2& val, float speed = 0.1f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::ivec3& val, float speed = 0.1f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::ivec4& val, float speed = 0.1f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::uvec2& val, float speed = 0.1f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::uvec3& val, float speed = 0.1f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::uvec4& val, float speed = 0.1f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);

	bool ControlColor(std::string_view label, glm::vec4& color);

	bool Control(std::string_view label, bool& val);
	bool ControlFlags(std::string_view label,  int16_t& val,  int16_t flag);
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag);
	bool ControlFlags(std::string_view label,  int32_t& val,  int32_t flag);
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag);

	bool ControlCombo(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount);
	bool ControlCombo(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount);
	bool ControlCombo(std::string_view label, int& index,      const std::string_view items[], uint32_t itemsCount);

	bool Control(std::string_view label, std::string& val);
	bool Control(std::string_view label, std::filesystem::path& path, const char* dragDropType = nullptr);
	bool Control(std::string_view label, UUID& uuid, const char* dragDropType = nullptr);
	bool ControlAssetPath(std::string_view label, AssetHandle& assetHandle, const char* dragDropType = DragDropID::Asset);
	bool ControlAssetPath(std::string_view label, std::filesystem::path& assetPath, const char* dragDropType = DragDropID::Asset);

	bool ControlCustomBegin(std::string_view label, TextFlags labelFlags = TextFlag::None);
	void ControlCustomEnd();

	void Property(std::string_view label, const char* text, TextFlags flags = TextFlag::None);
	void Property(std::string_view label, std::string_view text, TextFlags flags = TextFlag::None);
	void Property(std::string_view label, const std::string& text, TextFlags flags = TextFlag::None);
	void Property(std::string_view label, const std::filesystem::path& path, TextFlags flags = TextFlag::None);

	void Property(std::string_view label, const UUID& uuid);
	void Property(std::string_view label, float value);
	void Property(std::string_view label, int value);
	void Property(std::string_view label, uint32_t value);
	void Property(std::string_view label, bool value);

	void Property(std::string_view label, const glm::vec2& value);
	void Property(std::string_view label, TimeStep timestep);

	void PropertyColor(std::string_view label, const glm::vec4& color);

 	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Widgets ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Text(std::string_view str, TextFlags flags = TextFlag::None);
	void Text(const char* str,      TextFlags flags = TextFlag::None);

	void Text(const std::string& string, TextFlags flags = TextFlag::None);
	void Text(const std::filesystem::path& path, TextFlags flags = TextFlag::None);

	void TextSelectable(std::string_view str);

	void TextFramed(std::string_view fmt, ...);
	bool Search(ImGuiID id, char* buffer, int bufferSize);
	
	bool InputPath(const char* label, char* buffer, int bufferSize, bool& out_InvalidInput);

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
		~ScopedColor()                                           { ImGui::PopStyleColor(); }
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

		ScopedColorStack(ImGuiCol color, const ImVec4& val)
		{
			ImGui::PushStyleColor(color, val);
			m_Count++;
		}

		template<typename... TArgs>
		ScopedColorStack(ImGuiCol color, const ImVec4& val, TArgs&&... args)
			: ScopedColorStack(args...)
		{
			ImGui::PushStyleColor(color, val);
			m_Count++;
		}

		~ScopedColorStack()
		{
			PopAll();
		}

		void Push(ImGuiCol color, const ImVec4& val)
		{
			ImGui::PushStyleColor(color, val);
			m_Count++;
		}

		void Pop()
		{
			ImGui::PopStyleColor();
		}

		void PopAll()
		{
			ImGui::PopStyleColor(m_Count);
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

	struct ScopedFramedTextAlign
	{
		ScopedFramedTextAlign(const ImVec2& align)
		{
			PushFramedTextAlign(align);
		}
		~ScopedFramedTextAlign()
		{
			PopFramedTextAlign();
		}
	};

	struct UIControl
	{
		bool Active = false;
		uint32_t WidgetCount = 0;

		GridFlags ActiveGridFlags = GridFlag::None;
	};

	struct UIContext
	{
		UIControl Control;
		ImVec2 FramedTextAlign = ImVec2(0.0f, 0.0f);
		std::stack<ImVec2> FramedTextAlignStack;

		UIContext();
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// Init // Update /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	UIContext* CreateContext();
	void DestroyContext(UIContext* ctx = nullptr);

	void SetContext(UIContext* ctx);
	UIContext* GetContext();

	void NewFrame();

}
