#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "NodeGraph/EditorNodes.h"
#include "AnimationGraph/PinTypes.h"

namespace Shark::NodeGraph::Editor {

	struct AnimationTypes
	{
		enum EPinType : int
		{
			Domain = BIT(30),
			Pose,
		};

		template<int TPinType, typename TValueType, ImColor TColor = IM_COL32_WHITE>
		struct PinDescriptor
		{
			static constexpr auto PinType = TPinType;
			static constexpr auto Color = TColor;
			using value_type = TValueType;
		};

		using PinTypes = std::tuple<
			PinDescriptor<EPinType::Pose, Types::IPose, ImColor(255, 190, 100)>
		>;

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
			if (type.isObjectWithClassName("Pose"))
				return InitializePin(pin, EPinType::Pose);

			return false;
		}

	};

}
