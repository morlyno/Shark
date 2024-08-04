#pragma once

#include <imgui.h>

namespace Shark {
	class Project;
	class AssetManager;
	using AssetHandle = Shark::UUID;
	enum class AssetType;
	class Entity;
}

namespace Shark::UI {

	bool BeginControls();
	bool BeginControlsGrid();
	bool BeginControls(ImGuiID syncID);
	bool BeginControlsGrid(ImGuiID syncID);
	void EndControls();
	void EndControlsGrid();

	void ControlSetupColumns(std::string_view label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f);

	bool ControlHelperBegin(ImGuiID id);
	bool ControlHelperBegin(std::string_view strID);
	void ControlHelperEnd();
	void ControlHelperDrawLabel(std::string_view label);

	float ControlContentRegionWidth();

	bool Control(std::string_view label, float& val, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool Control(std::string_view label, double& val, float speed = 0.05f, double min = 0.0, double max = 0.0, const char* fmt = nullptr);
	bool ControlSlider(std::string_view label, float& val, float min = 0.0f, float max = 0.0f, const char* fmt = nullptr);
	bool ControlSlider(std::string_view label, double& val, double min = 0.0, double max = 0.0, const char* fmt = nullptr);

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

	bool ControlColor(std::string_view label, glm::vec3& color);
	bool ControlColor(std::string_view label, glm::vec4& color);

	bool Control(std::string_view label, bool& val);
	bool ControlFlags(std::string_view label, int16_t& val, int16_t flag);
	bool ControlFlags(std::string_view label, uint16_t& val, uint16_t flag);
	bool ControlFlags(std::string_view label, int32_t& val, int32_t flag);
	bool ControlFlags(std::string_view label, uint32_t& val, uint32_t flag);

	bool ControlCombo(std::string_view label, uint32_t& index, const std::string_view items[], uint32_t itemsCount);
	bool ControlCombo(std::string_view label, uint16_t& index, const std::string_view items[], uint32_t itemsCount);
	bool ControlCombo(std::string_view label, int& index, const std::string_view items[], uint32_t itemsCount);
	bool ControlCombo(std::string_view label, bool& value, const std::string_view falseValue, const std::string_view trueValue);

	template<typename TFunc>
	bool ControlCombo(std::string_view label, std::string_view preview, const TFunc& func);

	bool Control(std::string_view label, char* buffer, uint64_t bufferSize);
	bool Control(std::string_view label, std::string& val);
	bool Control(std::string_view label, std::filesystem::path& path);
	bool Control(std::string_view label, UUID& uuid);

	bool ControlAssetUnsave(std::string_view label, AssetHandle& assetHandle, const char* dragDropType = "Asset");
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const char* dragDropType = "Asset");

	template<typename TAsset>
	bool ControlAsset(std::string_view label, Ref<TAsset>& asset, const char* dragDropType = "Asset");
	bool ControlEntity(std::string_view label, UUID& entityID, const char* dragDropType = "Entity");

	bool ControlDragDrop(std::string_view label, std::string& val, const char* dragDropType);
	bool ControlDragDrop(std::string_view label, std::filesystem::path& val, const char* dragDropType);

	template<typename TFunc>
	void ControlCustom(std::string_view label, const TFunc& func);

	void Property(std::string_view label, const char* text);
	void Property(std::string_view label, std::string_view text);
	void Property(std::string_view label, const std::string& text);
	void Property(std::string_view label, const std::filesystem::path& path);

	void Property(std::string_view label, const UUID& uuid);
	void Property(std::string_view label, float value);
	void Property(std::string_view label, int value);
	void Property(std::string_view label, uint32_t value);
	void Property(std::string_view label, uint64_t value);
	void Property(std::string_view label, bool value);

	void Property(std::string_view label, const glm::vec2& value);
	void Property(std::string_view label, const glm::mat4& matrix);
	void Property(std::string_view label, TimeStep timestep);

	void PropertyColor(std::string_view label, const glm::vec4& color);

}

template<typename TFunc>
bool Shark::UI::ControlCombo(std::string_view label, std::string_view preview, const TFunc& func)
{
	if (!ControlHelperBegin(label))
		return false;

	ControlHelperDrawLabel(label);

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("#combo", preview.data()))
	{
		changed = func();
		ImGui::EndCombo();
	}

	ControlHelperEnd();
	return changed;
}

template<typename TFunc>
void Shark::UI::ControlCustom(std::string_view label, const TFunc& func)
{
	if (!ControlHelperBegin(label))
		return;

	ControlHelperDrawLabel(label);
	func();
	ControlHelperEnd();
}

template<typename TAsset>
bool Shark::UI::ControlAsset(std::string_view label, Ref<TAsset>& asset, const char* dragDropType)
{
	AssetHandle assetHandle = asset ? asset->Handle : AssetHandle::Invalid;
	if (ControlAsset(label, TAsset::GetStaticType(), assetHandle, dragDropType))
	{
		if (assetHandle == AssetHandle::Invalid)
		{
			asset = nullptr;
			return true;
		}

		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle);
		if (metadata.Type != TAsset::GetStaticType())
			return false;

		asset = AssetManager::GetAsset<TAsset>(assetHandle);
		return true;
	}
	return false;
}
