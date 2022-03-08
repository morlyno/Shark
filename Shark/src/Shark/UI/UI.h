#pragma once

#include "Shark/Core/Base.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/glm.hpp>

namespace ImGui {

	bool TableNextColumn(ImGuiTableRowFlags row_flags, float min_row_height);

}

namespace Shark::UI {

	enum
	{
		DefualtTreeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth,
		TreeNodeSeperatorFlags = DefualtTreeNodeFlags | ImGuiTreeNodeFlags_Selected
	};

	namespace Flags {

		enum TextEnum : uint16_t
		{
			Text_None = 0,
			Text_Aligned = BIT(0),
			Text_Selectable = BIT(1),
			Text_Disabled = BIT(2),
			Text_Background = BIT(3),

			Text_TagMask = BIT(0)
		};
		using Text = std::underlying_type_t<TextEnum>;

		enum GridEnum : uint16_t
		{
			GridNone = 0,
			GridInnerV = BIT(0),
			GridInnerH = BIT(1),
			GridOuterV = BIT(2),
			GridOuterH = BIT(3),

			GridDefault = BIT(4),
			GridOuterInner = GridInnerV | GridInnerH | GridOuterH,
			GridFull = GridInnerV | GridInnerH | GridOuterV | GridOuterH,
		};
		using Grid = std::underlying_type_t<GridEnum>;

	}

	void NewFrame();

	//////////////////////////////////////////////////////////////////////////////
	/// Helpers //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	struct ScopedID
	{
		ScopedID(ImGuiID id);
		ScopedID(int intID);
		ScopedID(void* ptrID);
		ScopedID(const char* strID);
		ScopedID(const std::string& strID);
		ScopedID(const std::string_view& strID);
		~ScopedID();
	};

	struct ScopedStyle
	{
		ScopedStyle() = default;
		ScopedStyle(ImGuiStyleVar idx, float val);
		ScopedStyle(ImGuiStyleVar idx, const ImVec2& val);
		ScopedStyle(ImGuiCol idx, const ImVec4& col);
		~ScopedStyle();

		void Push(ImGuiStyleVar idx, float val);
		void Push(ImGuiStyleVar idx, const ImVec2& val);
		void Push(ImGuiCol idx, const ImVec4& col);

		uint32_t StyleVarCount = 0;
		uint32_t StyleColorCount = 0;
	};

	ImGuiID GetID(int intID);
	ImGuiID GetID(void* ptrID);
	ImGuiID GetID(const char* strID);
	ImGuiID GetID(const std::string& strID);
	ImGuiID GetID(const std::string_view& strID);

	ImGuiID GetIDWithSeed(int intID, uint32_t seed);
	ImGuiID GetIDWithSeed(void* ptrID, uint32_t seed);
	ImGuiID GetIDWithSeed(const char* strID, uint32_t seed);
	ImGuiID GetIDWithSeed(const std::string& strID, uint32_t seed);

	void PushID(ImGuiID id);
	inline void PushID(int intID)                       { PushID(GetID(intID)); }
	inline void PushID(void* ptrID)                     { PushID(GetID(ptrID)); }
	inline void PushID(const char* strID)               { PushID(GetID(strID)); }
	inline void PushID(const std::string& strID)        { PushID(GetID(strID)); }
	inline void PushID(const std::string_view& strID)   { PushID(GetID(strID)); }
	ImGuiID PopID();

	ImGuiID GetCurrentID();

	void SetBlend(bool blend);

	void MoveCursor(const ImVec2& xy);
	void MoveCursorX(float x);
	void MoveCursorY(float y);

	void SpanAvailWith();

	const std::string& FormatToString(const std::wstring& str);
	const std::string& FormatToString(const std::filesystem::path& str);

	const char* FormatToCString(const std::wstring& str);
	const char* FormatToCString(const std::filesystem::path& str);

	//////////////////////////////////////////////////////////////////////////////
	/// Text /////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	void PushTextFlag(Flags::Text flags);
	void PopTextFlag(uint32_t count = 1);

	void Text(std::string_view str, Flags::Text flags);

	void Text(const char* str, Flags::Text flags);
	void Text(const std::string& str, Flags::Text flags);
	void Text(const std::filesystem::path& path, Flags::Text flags);

	void Text(const char* str);
	void Text(const std::string& str);
	void Text(const std::filesystem::path& path);

	void TextAligned(const char* str);
	void TextAligned(const std::string& str);
	void TextAligned(const std::filesystem::path& str);

	void TextWithBackGround(const std::string& text);
	void TextWithBackGround(const std::string& text, const ImVec4& bgColor);
	void TextWithBackGround(const std::filesystem::path& text);
	void TextWithBackGround(const std::filesystem::path& text, const ImVec4& bgColor);

	//////////////////////////////////////////////////////////////////////////////
	/// Controls /////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	bool BeginProperty();
	bool BeginProperty(const std::string& strID);
	bool BeginPropertyGrid(Flags::Grid flags = Flags::GridDefault);
	bool BeginPropertyGrid(const std::string& strID, Flags::Grid flags = Flags::GridDefault);

	bool BeginProperty(ImGuiID customID);
	bool BeginPropertyGrid(ImGuiID customID, Flags::Grid flags);

	void EndProperty();

	bool PropertyCustom(const std::string& tag);

	bool DragFloat(const std::string& tag, float& val,     float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, glm::vec2& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, glm::vec3& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragFloat(const std::string& tag, glm::vec4& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool SliderFloat(const std::string& tag, float& val,     float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, glm::vec2& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, glm::vec3& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderFloat(const std::string& tag, glm::vec4& val, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool DragInt(const std::string& tag, int& val,        int resetVal = 0.0f, int min = 0, int max = 0, float speed = 1, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, glm::ivec2& val, int resetVal = 0.0f, int min = 0, int max = 0, float speed = 1, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, glm::ivec3& val, int resetVal = 0.0f, int min = 0, int max = 0, float speed = 1, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragInt(const std::string& tag, glm::ivec4& val, int resetVal = 0.0f, int min = 0, int max = 0, float speed = 1, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool SliderInt(const std::string& tag, int& val,        int resetVal = 0, int min = 0, int max = 0, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, glm::ivec2& val, int resetVal = 0, int min = 0, int max = 0, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, glm::ivec3& val, int resetVal = 0, int min = 0, int max = 0, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, glm::ivec4& val, int resetVal = 0, int min = 0, int max = 0, const char* fmt = "%d", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool DragInt(const std::string& tag, uint32_t& val, uint32_t resetVal = 0, uint32_t min = 0, uint32_t max = 0, float speed = 1, const char* fmt = "%u", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool SliderInt(const std::string& tag, uint32_t& val, uint32_t resetVal = 0, uint32_t min = 0, uint32_t max = 0, const char* fmt = "%u", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	
	bool DragAngle(const std::string& tag, float& radians,     float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, glm::vec2& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, glm::vec3& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool DragAngle(const std::string& tag, glm::vec4& radians, float resetVal = 0.0f, float min = 0.0f, float max = 0.0f, float speed = 1.0f, const char* fmt = "%.2f", ImGuiSliderFlags flags = ImGuiSliderFlags_None);

	bool ColorEdit(const std::string& tag, glm::vec4& color, ImGuiColorEditFlags flags = ImGuiColorEditFlags_None);

	bool Checkbox(const std::string& tag, bool& v);
	bool Checkbox(const std::string& tag, const bool& v);

	void Property(const std::string& tag, const char* val, Flags::Text flags = Flags::Text_None);
	void Property(const std::string& tag, const std::string& val, Flags::Text flags = Flags::Text_None);
	void Property(const std::string& tag, const std::filesystem::path& path, Flags::Text flags = Flags::Text_None);

	bool BeginCustomControl(const std::string& strID);
	bool BeginCustomControl(ImGuiID id);
	void EndCustomControl();

	//////////////////////////////////////////////////////////////////////////////
	/// Widgets //////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	bool TreeNode(const std::string& tag, ImGuiTreeNodeFlags flags, ImTextureID textureID);

	void Separator(float size);

}
