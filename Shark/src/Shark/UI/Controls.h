#pragma once

#include "Shark/UI/UICore.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/UIUtilities.h"
#include "Shark/UI/ImGui/ImGuiHelpers.h"
#include <imgui.h>

namespace Shark {
	class Project;
	class AssetManager;
	using AssetHandle = Shark::UUID;
	enum class AssetType;
	class Entity;
	class Scene;
}

namespace Shark::UI {

	struct ControlSettings
	{
		bool ResetButton = false;
		float Reset = 0.0f;

		float Speed = 0.05f;
		float Min = 0.0f;
		float Max = 0.0f;
		const char* Format = nullptr;
	};

	bool ControlHelperBegin(ImGuiID id);
	void ControlHelperEnd();

	bool ControlHeader(std::string_view label, bool openByDefault = true, bool spanColumns = false);

	bool Control(std::string_view label, bool& val);
	void Control(std::string_view label, const bool& val);

	bool Control(std::string_view label, float& val, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, double& val, float speed = 0.05f, double min = 0.0, double max = 0.0, const char* fmt = nullptr);

	bool ControlSlider(std::string_view label, float& val, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool ControlSlider(std::string_view label, int32_t& val, int32_t min = 0.0f, int32_t max = 0.0f, const char* fmt = nullptr);
	bool ControlSlider(std::string_view label, uint32_t& val, uint32_t min = 0.0f, uint32_t max = 0.0f, const char* fmt = nullptr);

	bool Control(std::string_view label, int8_t& val, float speed = 0.05f, int8_t min = 0, int8_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int16_t& val, float speed = 0.05f, int16_t min = 0, int16_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int32_t& val, float speed = 0.05f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, int64_t& val, float speed = 0.05f, int64_t min = 0, int64_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, uint8_t& val, float speed = 0.05f, uint8_t min = 0, uint8_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, uint16_t& val, float speed = 0.05f, uint16_t min = 0, uint16_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, uint32_t& val, float speed = 0.05f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
	bool Control(std::string_view label, uint64_t& val, float speed = 0.05f, uint64_t min = 0, uint64_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::vec2& val, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::vec3& val, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::vec4& val, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::ivec2& val, float speed = 0.05f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::ivec3& val, float speed = 0.05f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::ivec4& val, float speed = 0.05f, int32_t min = 0, int32_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::uvec2& val, float speed = 0.05f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::uvec3& val, float speed = 0.05f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);
	bool Control(std::string_view label, glm::uvec4& val, float speed = 0.05f, uint32_t min = 0, uint32_t max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, glm::mat4& matrix, float speed = 0.05f, float min = 0, float max = 0, const char* fmt = nullptr);

	bool Control(std::string_view label, char* buffer, uint64_t bufferSize);
	bool Control(std::string_view label, const char* buffer, uint64_t bufferSize);
	bool Control(std::string_view label, std::string& val);
	bool Control(std::string_view label, const std::string& val);
	bool Control(std::string_view label, std::string_view val);

	bool ControlColor(std::string_view label, glm::vec3& color);
	bool ControlColor(std::string_view label, glm::vec4& color);

	glm::bvec2 Control(std::string_view label, glm::vec2& val, const glm::bvec2& isMixed, const ControlSettings& settings = {});
	glm::bvec3 Control(std::string_view label, glm::vec3& val, const glm::bvec3& isMixed, const ControlSettings& settings = {});
	glm::bvec3 ControlAngle(std::string_view label, glm::vec3& radians, const glm::bvec3& isMixed, const ControlSettings& settings = {});

	bool ControlCombo(std::string_view label, bool& value, const std::string_view falseValue, const std::string_view trueValue);

	template<typename TEnum>
	bool Control(std::string_view label, TEnum& selected) requires std::is_enum_v<TEnum>;

	template<typename TFunc>
	bool ControlCombo(std::string_view label, std::string_view preview, const TFunc& func);

	bool ControlAssetUnsave(std::string_view label, AssetHandle& assetHandle, const char* dragDropType = "Asset");
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle);
	bool ControlAsset(std::string_view label, std::string_view name, AssetType assetType, AssetHandle& assetHandle);

	struct AssetControlSettings
	{
		std::string_view DisplayName;
		ImU32 TextColor = Colors::Theme::Text;
		const char* DropType = "Asset";
		// TODO(moro): add preview image when hovering
	};
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const AssetControlSettings& settings);

	bool ControlEntity(std::string_view label, Ref<Scene> scene, UUID& entityID, const char* dragDropType = "Entity");
	bool ControlScript(std::string_view label, uint64_t& scriptID, const AssetControlSettings& settings);

	template<typename TFunc>
	auto ControlCustom(std::string_view label, const TFunc& func) -> std::invoke_result_t<TFunc>;

}

template<typename TEnum>
bool Shark::UI::Control(std::string_view label, TEnum& selected) requires std::is_enum_v<TEnum>
{
	if (!ControlHelperBegin(ImGui::GetID(label)))
		return false;

	ImGui::Text(label);
	ImGui::TableNextColumn();

	auto preview = magic_enum::enum_name(selected);
	bool modified = false;

	ImGui::SetNextItemWidth(-1.0f);
	if (UI::BeginCombo(UI::GenerateID(), preview.data()))
	{
		constexpr auto options = magic_enum::enum_entries<TEnum>();
		for (auto option : options)
		{
			const bool isSelected = option.first == selected;
			if (ImGui::Selectable(option.second.data(), isSelected))
			{
				selected = option.first;
				modified = true;
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		UI::EndCombo();
	}

	ControlHelperEnd();
	return modified;
}

template<typename TFunc>
bool Shark::UI::ControlCombo(std::string_view label, std::string_view preview, const TFunc& func)
{
	if (!ControlHelperBegin(ImGui::GetID(label)))
		return false;

	ImGui::Text(label);
	ImGui::TableNextColumn();

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (UI::BeginCombo("#combo", preview.data()))
	{
		changed = func();
		UI::EndCombo();
	}

	ControlHelperEnd();
	return changed;
}

template<typename TFunc>
auto Shark::UI::ControlCustom(std::string_view label, const TFunc& func) -> std::invoke_result_t<TFunc>
{
	static_assert(std::is_same_v<void, std::invoke_result_t<TFunc>> || std::is_same_v<bool, std::invoke_result_t<TFunc>>);
	if (!ControlHelperBegin(ImGui::GetID(label)))
		return (std::invoke_result_t<TFunc>)false;

	ImGui::Text(label);
	ImGui::TableNextColumn();

	bool changed = false;
	if constexpr (std::is_same_v<bool, std::invoke_result_t<TFunc>>)
		changed = func();
	else
		func();

	ControlHelperEnd();
	if constexpr (std::is_same_v<bool, std::invoke_result_t<TFunc>>)
		return changed;
}
