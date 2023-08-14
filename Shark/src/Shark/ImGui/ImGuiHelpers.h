#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Image.h"

#include <imgui.h>
#include <string>

namespace ImGui {

	bool TreeNode(ImTextureID textureID, const char* label, ImGuiTreeNodeFlags flags = 0);
	bool TreeNodeBehavior(ImTextureID textureID, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end);

	void ReadOnlyCheckbox(const char* label, bool value);

}
