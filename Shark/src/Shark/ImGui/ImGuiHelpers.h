#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Image.h"

#include <imgui.h>
#include <string>

namespace ImGui {

	void Text(const std::string& fmt, ...);

	bool TableNextColumn(ImGuiTableRowFlags row_flags, float min_row_height);

	bool TreeNode(ImTextureID textureID, const char* label, ImGuiTreeNodeFlags flags = 0);
	bool TreeNodeBehavior(ImTextureID textureID, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end);

	bool InputTextEx(ImGuiID id, const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* callback_user_data = NULL);

	void ReadOnlyCheckbox(const char* label, bool value);

}
