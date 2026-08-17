#include "skpch.h"
#include "AnimationFactory.h"

#include "Shark/Animation/Graph/Nodes/AnimationNodes.h"

#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/FactoryHelper.h"
#include "AnimationGraph/AnimationTypes.h"

namespace Shark::NodeGraph::Editor {

	template<auto TFunc>
		requires std::same_as<AnimationFactory*, std::tuple_element_t<0, Reflection::function_args_type<decltype(TFunc)>>>
	auto MakeFunction(AnimationFactory* factory)
	{
		return std::bind_front(TFunc, factory);
	}

	template<auto TFunc>
	auto MakeFunction(AnimationFactory* factory)
	{
		return TFunc;
	}

#define REG_CATEGORY(_name, ...) { _name, { __VA_ARGS__ } }
#define REG_NODE(_procNode) FactoryHelper<AnimationFactory>::CreateRegistryEntry<_procNode>(this)
#define REG_FUNC(_func) { #_func, MakeFunction<&_func>(this) }

	static void Output(AnimationFactory* factory, std::string_view category, Node& node)
	{
		node = Node("Output", category);
		node.Settings.CanEditPins = false;
		node.Inputs.emplace_back(factory->ConstructPin("Pose", AnimationTypes::EPinType::Pose));
		node.Initialize();
	}

	AnimationFactory::AnimationFactory()
	{
		Merge({
			{
				REG_CATEGORY(
					"Animation",
					REG_NODE(Nodes::AnimationPlayer),
					REG_FUNC(Output)
				)
			}
		});
	}

	bool AnimationFactory::InitializePin(Pin& outPin, int pinType) const
	{
		if (AnimationTypes::InitializePin(outPin, pinType))
			return true;
		return Base::InitializePin(outPin, pinType);
	}

	bool AnimationFactory::InitializePin(Pin& outPin, const std::type_info& type) const
	{
		if (AnimationTypes::InitializePin(outPin, type))
			return true;
		return Base::InitializePin(outPin, type);
	}

	bool AnimationFactory::InitializePin(Pin& outPin, const choc::value::Type& type) const
	{
		if (AnimationTypes::InitializePin(outPin, type))
			return true;
		return Base::InitializePin(outPin, type);
	}

	std::optional<int> AnimationFactory::GetPinTypeOverride(std::string_view node, std::string_view pin) const
	{
		if (node == "AnimationPlayer" && pin == "Pose")
			return AnimationTypes::EPinType::Pose;

		return Base::GetPinTypeOverride(node, pin);
	}

	choc::value::Type AnimationFactory::GetTypeFromPinType(int pinType) const
	{
		if (auto type = AnimationTypes::GetType(pinType, m_BoneCount); !type.isVoid())
			return type;

		return Base::GetTypeFromPinType(pinType);
	}

}
