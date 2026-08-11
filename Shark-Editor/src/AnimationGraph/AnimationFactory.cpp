#include "skpch.h"
#include "AnimationFactory.h"

#include "Shark/Animation/Graph/Nodes/AnimationNodes.h"

#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/FactoryHelper.h"
#include "AnimationGraph/AnimationTypes.h"

namespace Shark::NodeGraph::Editor {

	template<typename... TProcNodes>
	static std::pair<std::string, std::map<std::string, FactoryEntry, std::ranges::less>> CreateRegistryCategory(AnimationFactory* factory, std::string category)
	{
		return {
			std::move(category),
			{
				FactoryHelper<AnimationFactory>::CreateRegistryEntry<TProcNodes>(factory)...
			}
		};
	}

	static void Output(std::string_view category, Node& node)
	{
		node = Node("Output", category);
		node.Settings.CanEditPins = false;
		node.Inputs.emplace_back("Pose", AnimationTypes::EPinType::Pose, ImColor(255, 190, 100));
		node.Initialize();
	}

	AnimationFactory::AnimationFactory()
	{
		Merge(
			FactoryRegistry{
				CreateRegistryCategory<
					Nodes::AnimationPlayer
				>(this, "Animation")
			}
		);

		Merge({
			{
				"Animation",
				{
					{ "Output", FactoryEntry(&Output, nullptr) },
				}
			}
		});
	}

	bool AnimationFactory::InitializePin(Pin& outPin, int pinType) const
	{
		if (!AnimationTypes::InitializePin(outPin, pinType))
			return Base::InitializePin(outPin, pinType);
		return false;
	}

	bool AnimationFactory::InitializePin(Pin& outPin, const std::type_info& type) const
	{
		if (!AnimationTypes::InitializePin(outPin, type))
			return Base::InitializePin(outPin, type);
		return false;
	}

	bool AnimationFactory::InitializePin(Pin& outPin, const choc::value::Type& type) const
	{
		if (!AnimationTypes::InitializePin(outPin, type))
			return Base::InitializePin(outPin, type);
		return false;
	}

	std::optional<int> AnimationFactory::GetPinTypeOverride(std::string_view node, std::string_view pin) const
	{
		if (node == "AnimationPlayer" && pin == "Pose")
			return AnimationTypes::EPinType::Pose;

		return Base::GetPinTypeOverride(node, pin);
	}

}
