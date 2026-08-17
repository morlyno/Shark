#include "skpch.h"
#include "Factory.h"

#include "Shark/NodeGraph/Nodes/MathNodes.h"
#include "Shark/NodeGraph/Nodes/EntityNodes.h"
#include "Shark/NodeGraph/Nodes/TriggerNodes.h"
#include "Shark/NodeGraph/Nodes/DebugNodes.h"

#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/CoreTypes.h"
#include "NodeGraph/FactoryHelper.h"

#include <choc/text/choc_StringUtilities.h>

namespace Shark::NodeGraph::Editor {

	bool AbstractFactory::SpawnNode(std::string_view category, std::string_view type, Node& outNode) const
	{
		const auto cat = m_Registry.find(category);
		if (cat != m_Registry.end())
		{
			const auto entry = cat->second.find(type);
			if (entry != cat->second.end())
			{
				entry->second(category, outNode);
				return true;
			}
		}
		return false;
	}

	void AbstractFactory::Merge(FactoryRegistry registry)
	{
		m_Registry.merge(registry);

		for (auto& [category, list] : registry)
			m_Registry[category].merge(list);

		SK_CORE_VERIFY(registry.empty() || std::ranges::all_of(registry, [](auto& entry) { return entry.second.empty(); }));
	}

	Pin AbstractFactory::ConstructPin(std::string_view name, int pinType) const
	{
		std::string_view prefixlessName = Details::RemovePinPrefix(name);
		std::string friendlyName = Details::CreateFriendlyPinName(name);

		Pin pin;
		InitializePin(pin, pinType);
		pin.Name = friendlyName;
		pin.Identifier = prefixlessName;
		return pin;
	}

	template<typename TProcNode>
	static std::pair<std::string, std::function<NodeSpawnerSignature>> CreateRegistryEntry(CoreFactory* factory, const NodeSettings& settings = {})
	{
		return {
			choc::text::replace(NodeType<TProcNode>::Inputs::Class, "<", " (", ">", ")"),
			std::bind_front(FactoryHelper<CoreFactory>::ConstructNode<TProcNode>, factory, settings),
		};
	}

	static FactoryRegistry CoreRegistry(CoreFactory* factory)
	{
		#define FACTORY_NODE(_procNode, ...) CreateRegistryEntry<_procNode>(factory __VA_OPT__(,) __VA_ARGS__)
		
		const NodeSettings Simple       = { .IsSimple = true };
		const NodeSettings SimpleNoEdit = { .IsSimple = true, .CanEditPins = false };
		const NodeSettings NoEdit       = { .CanEditPins = false };

		return FactoryRegistry{
			{
				"Math",
				{
					FACTORY_NODE(Nodes::Add<int>, Simple),
					FACTORY_NODE(Nodes::Add<float>, Simple),
					FACTORY_NODE(Nodes::Multiply<int>, Simple),
					FACTORY_NODE(Nodes::Multiply<float>, Simple),
					FACTORY_NODE(Nodes::Get, SimpleNoEdit),
					FACTORY_NODE(Nodes::Random<int>),
					FACTORY_NODE(Nodes::Random<float>),
				}
			},
			{
				"Trigger",
				{
					FACTORY_NODE(Nodes::BoolTrigger),
				}
			},
			{
				"Scene",
				{
					FACTORY_NODE(Nodes::EntityTransform)
				}
			},
			{
				"Debug",
				{
					FACTORY_NODE(Nodes::Test)
				}
			}
		};

		#undef FACTORY_NODE
	}

	CoreFactory::CoreFactory()
	{
		Merge(CoreRegistry(this));
	}

	bool CoreFactory::InitializePin(Pin& outPin, int pinType) const
	{
		return CoreTypes::InitializePin(outPin, pinType);
	}

	bool CoreFactory::InitializePin(Pin& outPin, const std::type_info& type) const
	{
		return CoreTypes::InitializePin(outPin, type);
	}

	bool CoreFactory::InitializePin(Pin& outPin, const choc::value::Type& type) const
	{
		return CoreTypes::InitializePin(outPin, type);
	}

	choc::value::Type CoreFactory::GetTypeFromPinType(int pinType) const
	{
		return CoreTypes::GetType(pinType);
	}

	std::optional<int> CoreFactory::GetPinTypeOverride(std::string_view node, std::string_view pin) const
	{
		return std::nullopt;
	}

}
