#pragma once

#include "Shark/Core/UUID.h"
#include <imgui.h>

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

struct ImRect;

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

	void PushClipRect(const ImRect& clip_rect, bool intersect_with_current_clip_rect = false);

	bool BeginPopupModalEx(ImGuiID id, const char* name, bool* p_open, ImGuiWindowFlags flags);

}
