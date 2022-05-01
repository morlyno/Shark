#pragma once

#include "Shark/Core/Base.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>


#ifdef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec4 operator*(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs); }
static inline ImVec4 operator/(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs); }
#endif

namespace ImGui {

	bool TableNextColumn(ImGuiTableRowFlags row_flags, float min_row_height);

}

inline std::ostream& operator<<(std::ostream& out, const ImVec2& v)
{
	return out << fmt::format("[{}, {}]", v.x, v.y);
}

namespace Shark::UI {

	struct UIControl;
	struct UIContext;

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Flags //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum
	{
		DefualtTreeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth,
		TreeNodeSeperatorFlags = DefualtTreeNodeFlags | ImGuiTreeNodeFlags_Selected
	};

	enum class ControlType : uint16_t
	{
		Drag,
		Slider
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
	inline ImGuiID GetID(const char* strID)                                    { return GImGui->CurrentWindow->GetID(strID); }
	inline ImGuiID GetID(const std::string& strID)                             { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }
	inline ImGuiID GetID(const std::string_view& strID)                        { return GImGui->CurrentWindow->GetID(strID.data(), strID.data() + strID.size()); }

	inline ImGuiID GetIDWithSeed(int intID, uint32_t seed)                     { const ImGuiID id = ImHashData(&intID, sizeof(int), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(void* ptrID, uint32_t seed)                   { const ImGuiID id = ImHashData(&ptrID, sizeof(void*), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(const char* strID, uint32_t seed)             { const ImGuiID id = ImHashStr(strID, 0, seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(const std::string& strID, uint32_t seed)      { const ImGuiID id = ImHashStr(strID.c_str(), strID.size(), seed); ImGui::KeepAliveID(id); return id; }
	inline ImGuiID GetIDWithSeed(std::string_view strID, uint32_t seed)        { const ImGuiID id = ImHashStr(strID.data(), strID.size(), seed); ImGui::KeepAliveID(id); return id; }

	inline void PushID(ImGuiID id)                                             { GImGui->CurrentWindow->IDStack.push_back(id) ;}
	inline void PushID(int intID)                                              { PushID(GetID(intID)); }
	inline void PushID(void* ptrID)                                            { PushID(GetID(ptrID)); }
	inline void PushID(const char* strID)                                      { PushID(GetID(strID)); }
	inline void PushID(const std::string& strID)                               { PushID(GetID(strID)); }
	inline void PushID(const std::string_view& strID)                          { PushID(GetID(strID)); }
	inline void PopID()                                                        { GImGui->CurrentWindow->IDStack.pop_back(); }

	inline ImGuiID GetCurrentID()                                              { return GImGui->CurrentWindow->IDStack.back(); }

	inline void MoveCursor(const ImVec2& delta)                                { ImGui::SetCursorPos(ImGui::GetCursorPos() + delta); }
	inline void MoveCursorX(float deltaX)                                      { MoveCursor({ deltaX, 0.0f }); }
	inline void MoveCursorY(float deltaY)                                      { MoveCursor({ 0.0f, deltaY }); }

	void SetBlend(bool blend);

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Controls ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool BeginControls();
	bool BeginControlsGrid(GridFlags flags = GridFlag::Label);
	bool BeginControls(ImGuiID syncID);
	bool BeginControlsGrid(ImGuiID syncID, GridFlags flags = GridFlag::Label);
	void EndControls();
	void EndControlsGrid();

	bool ControlBeginHelper(ImGuiID id);
	bool ControlBeginHelper(std::string_view strID);
	void ControlEndHelper();

	bool Control(std::string_view label, float& val,     float resetVal            = float(0),     float min            = float(0),     float max            = float(0),     float speed            = float(1),     std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::vec2& val, const glm::vec2& resetVal = glm::vec2(0), const glm::vec2& min = glm::vec2(0), const glm::vec2& max = glm::vec2(0), const glm::vec2& speed = glm::vec2(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::vec3& val, const glm::vec3& resetVal = glm::vec3(0), const glm::vec3& min = glm::vec3(0), const glm::vec3& max = glm::vec3(0), const glm::vec3& speed = glm::vec3(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::vec4& val, const glm::vec4& resetVal = glm::vec4(0), const glm::vec4& min = glm::vec4(0), const glm::vec4& max = glm::vec4(0), const glm::vec4& speed = glm::vec4(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);

	bool Control(std::string_view label, int& val,        int resetVal               = int(0),        int min               = int(0),        int max               = int(0),        int speed               = int(1),        std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::ivec2& val, const glm::ivec2& resetVal = glm::ivec2(0), const glm::ivec2& min = glm::ivec2(0), const glm::ivec2& max = glm::ivec2(0), const glm::ivec2& speed = glm::ivec2(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::ivec3& val, const glm::ivec3& resetVal = glm::ivec3(0), const glm::ivec3& min = glm::ivec3(0), const glm::ivec3& max = glm::ivec3(0), const glm::ivec3& speed = glm::ivec3(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::ivec4& val, const glm::ivec4& resetVal = glm::ivec4(0), const glm::ivec4& min = glm::ivec4(0), const glm::ivec4& max = glm::ivec4(0), const glm::ivec4& speed = glm::ivec4(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);

	bool Control(std::string_view label, uint32_t& val,   uint32_t resetVal          = uint32_t(0),   uint32_t min          = uint32_t(0),   uint32_t max          = uint32_t(0),   uint32_t speed          = uint32_t(1),   std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::uvec2& val, const glm::uvec2& resetVal = glm::uvec2(0), const glm::uvec2& min = glm::uvec2(0), const glm::uvec2& max = glm::uvec2(0), const glm::uvec2& speed = glm::uvec2(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::uvec3& val, const glm::uvec3& resetVal = glm::uvec3(0), const glm::uvec3& min = glm::uvec3(0), const glm::uvec3& max = glm::uvec3(0), const glm::uvec3& speed = glm::uvec3(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool Control(std::string_view label, glm::uvec4& val, const glm::uvec4& resetVal = glm::uvec4(0), const glm::uvec4& min = glm::uvec4(0), const glm::uvec4& max = glm::uvec4(0), const glm::uvec4& speed = glm::uvec4(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);

	bool ControlAngle(std::string_view label, float& radians,     float resetVal            = float(0),     float min            = float(0),     float max            = float(0),     float speed            = float(1),     std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool ControlAngle(std::string_view label, glm::vec2& radians, const glm::vec2& resetVal = glm::vec2(0), const glm::vec2& min = glm::vec2(0), const glm::vec2& max = glm::vec2(0), const glm::vec2& speed = glm::vec2(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool ControlAngle(std::string_view label, glm::vec3& radians, const glm::vec3& resetVal = glm::vec3(0), const glm::vec3& min = glm::vec3(0), const glm::vec3& max = glm::vec3(0), const glm::vec3& speed = glm::vec3(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);
	bool ControlAngle(std::string_view label, glm::vec4& radians, const glm::vec4& resetVal = glm::vec4(0), const glm::vec4& min = glm::vec4(0), const glm::vec4& max = glm::vec4(0), const glm::vec4& speed = glm::vec4(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag);

	bool ControlColor(std::string_view label, glm::vec4& color);

	bool Control(std::string_view label, bool& val);
	bool ControlFlags(std::string_view label,  int16_t& val,  int16_t flag);
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag);
	bool ControlFlags(std::string_view label,  int32_t& val,  int32_t flag);
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag);

	bool Control(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount);
	bool Control(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount);
	bool Control(std::string_view label, int& index,      const std::string_view items[], uint32_t itemsCount);

	void Control(std::string_view label, std::string_view str, TextFlags flags = TextFlag::None, TextFlags labelFlags = TextFlag::None);
	void Control(std::string_view label, std::string&& str,    TextFlags flags = TextFlag::None, TextFlags labelFlags = TextFlag::None);
	void Control(std::string_view label, const char* str,      TextFlags flags = TextFlag::None, TextFlags labelFlags = TextFlag::None);

	void Control(std::string_view label, const std::filesystem::path& filePath, TextFlags flags = TextFlag::None, TextFlags labelFalgs = TextFlag::None);
	void Control(std::string_view label, const std::string& filePath,           TextFlags flags = TextFlag::None, TextFlags labelFalgs = TextFlag::None);

	bool ControlCustomBegin(std::string_view label, TextFlags labelFlags = TextFlag::None);
	void ControlCustomEnd();

	void Control(ImGuiID id, void(*func)(void* data, ImGuiID id), void* data = nullptr);
	void Control(std::string_view label, void(*func)(void* data, ImGuiID id), void* data = nullptr);

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Helpers ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline bool ControlS(std::string_view label, glm::vec2& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::vec2(resetVal), glm::vec2(speed), glm::vec2(min), glm::vec2(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::vec3& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::vec3(resetVal), glm::vec3(speed), glm::vec3(min), glm::vec3(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::vec4& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::vec4(resetVal), glm::vec4(speed), glm::vec4(min), glm::vec4(max), fmt, type); }

	inline bool ControlS(std::string_view label, glm::ivec2& val, int resetVal = int(0), int min = int(0), int max = int(0), int speed = int(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::ivec2(resetVal), glm::ivec2(speed), glm::ivec2(min), glm::ivec2(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::ivec3& val, int resetVal = int(0), int min = int(0), int max = int(0), int speed = int(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::ivec3(resetVal), glm::ivec3(speed), glm::ivec3(min), glm::ivec3(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::ivec4& val, int resetVal = int(0), int min = int(0), int max = int(0), int speed = int(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::ivec4(resetVal), glm::ivec4(speed), glm::ivec4(min), glm::ivec4(max), fmt, type); }

	inline bool ControlS(std::string_view label, glm::uvec2& val, uint32_t resetVal = uint32_t(0), uint32_t min = uint32_t(0), uint32_t max = uint32_t(0), uint32_t speed = uint32_t(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::uvec2(resetVal), glm::uvec2(speed), glm::uvec2(min), glm::uvec2(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::uvec3& val, uint32_t resetVal = uint32_t(0), uint32_t min = uint32_t(0), uint32_t max = uint32_t(0), uint32_t speed = uint32_t(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::uvec3(resetVal), glm::uvec3(speed), glm::uvec3(min), glm::uvec3(max), fmt, type); }
	inline bool ControlS(std::string_view label, glm::uvec4& val, uint32_t resetVal = uint32_t(0), uint32_t min = uint32_t(0), uint32_t max = uint32_t(0), uint32_t speed = uint32_t(1), std::string_view fmt = std::string_view{}, ControlType type = ControlType::Drag) { return Control(label, val, glm::uvec4(resetVal), glm::uvec4(speed), glm::uvec4(min), glm::uvec4(max), fmt, type); }

 	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Widgets ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Text(std::string_view str, TextFlags flags = TextFlag::None);
	void Text(std::string&& str,    TextFlags flags = TextFlag::None);
	void Text(const char* str,      TextFlags flags = TextFlag::None);

	void Text(const std::filesystem::path& filePath, TextFlags flags = TextFlag::None);
	void Text(const std::string& str,                TextFlags flags = TextFlag::None);

	void TextSelectable(std::string_view str);

	 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Types //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct ScopedID
	{
		ScopedID(ImGuiID id)                               { PushID(id); }
		ScopedID(int intID)                                { PushID(intID); }
		ScopedID(void* ptrID)                              { PushID(ptrID); }
		ScopedID(const char* strID)                        { PushID(strID); }
		ScopedID(const std::string& strID)                 { PushID(strID); }
		ScopedID(const std::string_view& strID)            { PushID(strID); }
		~ScopedID()                                        { PopID(); }
	};

	struct ScopedStyle
	{
		ScopedStyle() = default;
		ScopedStyle(ImGuiStyleVar idx, float val)          { Push(idx, val); }
		ScopedStyle(ImGuiStyleVar idx, const ImVec2& val)  { Push(idx, val); }
		ScopedStyle(ImGuiCol idx, const ImVec4& col)       { Push(idx, col); }
		~ScopedStyle()                                     { Pop(); }

		void Pop()                                         { ImGui::PopStyleVar(VarCount); ImGui::PopStyleColor(ColorCount); VarCount = 0; ColorCount = 0; }
		void PopVar(int count = -1)                        { ImGui::PopStyleVar(count); count == -1 ? VarCount = 0 : VarCount -= count; }
		void PopColor(int count = -1)                      { ImGui::PopStyleColor(count); count == -1 ? ColorCount = 0 : ColorCount -= count; }

		void Push(ImGuiStyleVar idx, float val)            { ImGui::PushStyleVar(idx, val); VarCount++; }
		void Push(ImGuiStyleVar idx, const ImVec2& val)    { ImGui::PushStyleVar(idx, val); VarCount++; }
		void Push(ImGuiCol idx, const ImVec4& col)         { ImGui::PushStyleColor(idx, col); ColorCount++; }

		uint32_t VarCount = 0;
		uint32_t ColorCount = 0;
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

		std::string_view DefaultFormat[ImGuiDataType_COUNT];

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
