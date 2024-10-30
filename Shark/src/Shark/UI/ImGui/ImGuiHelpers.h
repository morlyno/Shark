#pragma once

#include <imgui.h>

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

namespace ImGui {

	ImGuiID GetID(const std::string& strID);
	ImGuiID GetID(std::string_view strID);
	ImGuiID GetID(Shark::UUID uuid);

	ImGuiID GetIDWithSeed(const void* ptr, ImGuiID seed);
	ImGuiID GetIDWithSeed(Shark::UUID uuid, ImGuiID seed);

	void PushID(const std::string& strID);
	void PushID(std::string_view strID);
	void PushID(Shark::UUID uuid);

	void Text(std::string_view str);
	void Text(const std::string& string);
	void Text(const std::filesystem::path& path);

	bool BeginDrapDropTargetWindow(const char* payload_type = NULL);

	bool BeginPopupModal(ImGuiID id, const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);

}
