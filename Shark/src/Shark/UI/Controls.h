#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"
#include "Shark/UI/UICore.h"
#include "Shark/Asset/Asset.h"

namespace Shark {
	class Scene;
}

namespace Shark::UI {

	bool Drag(const char* label, float&    v, float v_speed = 1.0f, float    v_min = 0.0f, float v_max    = 0.0f, const char* format = "%.3f",  ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, double&   v, float v_speed = 1.0f, double   v_min = 0.0,  double v_max   = 0.0,  const char* format = "%.3f",  ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, int8_t&   v, float v_Speed = 1.0f, int8_t   v_min = 0,    int8_t v_max   = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, int16_t&  v, float v_Speed = 1.0f, int16_t  v_min = 0,    int16_t v_max  = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, int32_t&  v, float v_Speed = 1.0f, int32_t  v_min = 0,    int32_t v_max  = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, int64_t&  v, float v_Speed = 1.0f, int64_t  v_min = 0,    int64_t v_max  = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, uint8_t&  v, float v_Speed = 1.0f, uint8_t  v_min = 0,    uint8_t v_max  = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, uint16_t& v, float v_Speed = 1.0f, uint16_t v_min = 0,    uint16_t v_max = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, uint32_t& v, float v_Speed = 1.0f, uint32_t v_min = 0,    uint32_t v_max = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, uint64_t& v, float v_Speed = 1.0f, uint64_t v_min = 0,    uint64_t v_max = 0,    const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool Slider(const char* label, float&    v, float    v_min, float    v_max, const char* format = "%.3f",  ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, double&   v, double   v_min, double   v_max, const char* format = "%.3f",  ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, int8_t&   v, int8_t   v_min, int8_t   v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, int16_t&  v, int16_t  v_min, int16_t  v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, int32_t&  v, int32_t  v_min, int32_t  v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, int64_t&  v, int64_t  v_min, int64_t  v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, uint8_t&  v, uint8_t  v_min, uint8_t  v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, uint16_t& v, uint16_t v_min, uint16_t v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, uint32_t& v, uint32_t v_min, uint32_t v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, uint64_t& v, uint64_t v_min, uint64_t v_max, const char* format = nullptr, ImGuiSliderFlags flags = 0);

	bool Drag(const char* label, glm::vec2& v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, glm::vec3& v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool Drag(const char* label, glm::vec4& v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	bool Slider(const char* label, glm::vec2& v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, glm::vec3& v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	bool Slider(const char* label, glm::vec4& v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	template<typename T>
	struct ControlArgs
	{
		float Speed = 0.05f;
		T Min = static_cast<T>(0);
		T Max = static_cast<T>(0);
		const char* Format = nullptr;
		bool Slider = false;
	};

	namespace details {

		template<typename T>
		concept Modifiable = requires(T & value)
		{
			{ Drag(nullptr, value) };
		};

		template<typename T>
		struct MapT { using type = T; };

		template<size_t L, typename T>
		struct MapT<glm::vec<L, T>> { using type = T; };

		template<typename T>
		using TMapped = typename MapT<T>::type;

		struct SpeedArg
		{
			float Speed = 0.005f;

			template<typename T>
			operator ControlArgs<T>() { return ControlArgs<T>{.Speed = Speed }; }
		};

	}

	struct as_color_t {} static constexpr as_color;
	// #TODO struct as_degrees_t{} static constexpr as_degrees;

	template<typename T>
	constexpr auto as_drag(T min, T max)   { return ControlArgs<T>{ .Min = min, .Max = max }; }
	constexpr auto as_drag(float speed)    { return details::SpeedArg{ .Speed = speed }; }

	template<typename T>
	constexpr auto as_slider(T min, T max) { return ControlArgs<T>{ .Min = min, .Max = max, .Slider = true }; }


	template<details::Modifiable T>
	bool Control(std::string_view label, T& value, const ControlArgs<details::TMapped<T>>& args = {});
	template<details::Modifiable T>
	bool Control(std::string_view label, const T& value, const ControlArgs<details::TMapped<T>>& args = {});

	bool Control(std::string_view label, glm::vec3& value, as_color_t);
	bool Control(std::string_view label, glm::vec4& value, as_color_t);
	bool Control(std::string_view label, bool& value);
	bool Control(std::string_view label, const bool& value);
	bool Control(std::string_view label, bool& value, const char* vTrue, const char* vFalse);
	bool Control(std::string_view label, Concepts::Enum auto& value);
	
	bool Control(std::string_view label, char* buffer, size_t bufferSize);
	bool Control(std::string_view label, std::string& value);
	bool Control(std::string_view label, std::string_view value);

	bool Control(std::string_view label, std::invocable auto&& func);
	
	struct EntityControlArgs
	{
		std::string_view DisplayName = {};
		ImU32            TextColor   = Colors::Theme::Text;
		const char*      DropType    = "Entity";
	};

	struct AssetControlArgs
	{
		std::string_view DisplayName = {};
		ImU32            TextColor   = Colors::Theme::Text;
		const char*      DropType    = "Asset";
	};

	bool ControlEntity(std::string_view label, Ref<Scene> scene, UUID& entityID, const EntityControlArgs& args = {});
	bool ControlAsset(std::string_view label, AssetType assetType, AssetHandle& assetHandle, const AssetControlArgs& args = {});
	bool ControlScript(std::string_view label, uint64_t& scriptID, const AssetControlArgs& args = {});

	// #TODO move to UICore.h
	inline bool ControlHeader(std::string_view label, bool openByDefault = true, bool spanColumns = false)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
		if (openByDefault)
			treeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (spanColumns)
			treeFlags |= ImGuiTreeNodeFlags_SpanAllColumns;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		return ImGui::TreeNodeEx(label.data(), treeFlags);
	}

}

namespace Shark::UI {

	namespace details {

		void GridSeparator();
		bool BeginControl(ImGuiID id);
		void EndControl();

	}

	template<details::Modifiable T>
	bool Control(std::string_view label, T& value, const ControlArgs<details::TMapped<T>>& args)
	{
		if (!details::BeginControl(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		ImGui::SetNextItemWidth(-1.0f);
		bool modified = false;

		if (args.Slider)
			modified = Slider(GenerateID(), value, args.Min, args.Max, args.Format);
		else
			modified = Drag(GenerateID(), value, args.Speed, args.Min, args.Max, args.Format);

		details::EndControl();
		return modified;
	}

	template<details::Modifiable T>
	bool Control(std::string_view label, const T& value, const ControlArgs<details::TMapped<T>>& args)
	{
		UI::ScopedItemFlag readOnly(ImGuiItemFlags_ReadOnly);

		auto temp = value;
		Control(label, temp, args);
		return false;
	}

	bool Control(std::string_view label, Concepts::Enum auto& value)
	{
		if (!details::BeginControl(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		auto preview = magic_enum::enum_name(value);
		bool modified = false;

		ImGui::SetNextItemWidth(-1.0f);
		if (BeginCombo(GenerateID(), preview.data()))
		{
			constexpr auto options = magic_enum::enum_entries<decltype(value)>();
			for (auto option : options)
			{
				const bool isSelected = option.first == value;
				if (ImGui::Selectable(option.second.data(), isSelected))
				{
					value = option.first;
					modified = true;
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			EndCombo();
		}
		details::EndControl();
		return modified;
	}

	bool Control(std::string_view label, std::invocable auto&& func)
	{
		if (!details::BeginControl(ImGui::GetID(label)))
			return false;

		ImGui::Text(label);
		ImGui::TableNextColumn();

		const bool result = func();

		details::EndControl();
		return result;
	}

}
