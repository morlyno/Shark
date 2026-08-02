#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/PinTypes.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>
#include <choc/text/choc_StringUtilities.h>

#include <tuple>

namespace Shark::NodeGraph::Editor {

	struct CoreTypes
	{
		enum EPinType : int
		{
			Flow = 0, // Flow must stay at 0
			Bool,
			Int,
			Float,
			Vec3,
			EntityID,
			AssetHandle
		};

		template<int TPinType, typename TValueType, ImColor TColor = IM_COL32_WHITE>
		struct PinDescriptor
		{
			static constexpr auto PinType = TPinType;
			static constexpr auto Color = TColor;
			using value_type = TValueType;
		};

		using PinTypes = std::tuple<
			PinDescriptor<EPinType::Flow,        Types::Flow,        ImColor(255, 255, 255)>,
			PinDescriptor<EPinType::Bool,        bool,               ImColor(220,  48,  48)>,
			PinDescriptor<EPinType::Int,         int,                ImColor( 68, 201, 156)>,
			PinDescriptor<EPinType::Float,       float,              ImColor(147, 226,  74)>,
			PinDescriptor<EPinType::Vec3,        glm::vec3,          ImColor(147, 226,  74)>,
			PinDescriptor<EPinType::EntityID,    Types::EntityID,    ImColor( 51, 150, 215)>,
			PinDescriptor<EPinType::AssetHandle, Types::AssetHandle, ImColor(215, 150,  51)>
		>;

		template<typename TMemberPtr>
		static EPinType GetPinTypeFromMember()
		{
			using TMember = Reflection::member_return_type<TMemberPtr>;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			if constexpr (std::is_member_function_pointer_v<TMemberPtr> || std::is_same_v<TMemberPtr, ProcessNode::OutputEvent>)
				return EPinType::Flow;

			EPinType pinType = EPinType::Flow;

			Reflection::ForEach(PinTypes{}, [&pinType]<typename TDesc>()
			{
				if constexpr (std::is_same_v<const TDesc::value_type, const TMemberRaw>)
				{
					pinType = static_cast<EPinType>(TDesc::PinType);
				}
			});

			return pinType;
		}

		template<typename TType>
		static bool InitializePin(Pin& pin, const TType& type)
		{
			bool initialized = false;

			Reflection::ForEach(PinTypes{}, [&pin, &type, &initialized]<typename TDesc>()
			{
				bool same = false;
				if constexpr (std::is_same_v<TType, EPinType> || std::is_same_v<TType, int>)
				{
					same = TDesc::PinType == type;
				}
				else if constexpr (std::is_same_v<TType, std::type_info>)
				{
					same = typeid(TDesc::value_type) == type;
				}
				else
				{
					static_assert(false);
				}

				if (same)
				{
					pin.SetDesc<TDesc>();
					initialized = true;
				}
			});

			return initialized;
		}

		static bool InitializePin(Pin& pin, const choc::value::Type& type)
		{
			if (type.isObjectWithClassName("Flow"))
				return InitializePin(pin, EPinType::Flow);

			if (type.isObjectWithClassName("EntityID"))
				return InitializePin(pin, EPinType::EntityID);

			if (type.isObjectWithClassName("AssetHandle"))
				return InitializePin(pin, EPinType::AssetHandle);

			if (type.isBool())
				return InitializePin(pin, EPinType::Bool);
			if (type.isInt32())
				return InitializePin(pin, EPinType::Int);
			if (type.isFloat32())
				return InitializePin(pin, EPinType::Float);

			SK_CORE_ASSERT(false, "Unknown Type");
			return false;
		}

	};


}
