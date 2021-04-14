#pragma once

#include "Shark/Render/FrameBuffer.h"

#include <imgui.h>

#include <DirectXMath.h>

namespace Shark::UI {

	void DrawFloatShow(const char* lable, float val, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacter = "X");

	void DrawVec2Show(const char* lable, DirectX::XMFLOAT2 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacters = "X Y");

	void DrawVec3Show(const char* lable, DirectX::XMFLOAT3 vec, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacters = "X Y Z");

	bool DrawFloatControl(const char* lable, float& val, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacter = "X");

	bool DrawVec2Control(const char* lable, DirectX::XMFLOAT2& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacters = "X Y");

	bool DrawVec3Control(const char* lable, DirectX::XMFLOAT3& vec, float resetVal = 0.0f, const char* fmt = "%.2f", float textWidth = 100.0f, const std::string& buttoncharacters = "X Y Z");

	void NoAlpaImage(const Ref<FrameBuffer>& framebuffer, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = { 0, 0 }, const ImVec2& uv1 = { 1, 1 }, const ImVec4& tintcolor = { 1, 1, 1, 1 }, const ImVec4& bordercolor = { 0, 0 , 0, 0 });

}
