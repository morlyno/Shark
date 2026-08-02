#include "skpch.h"
#include "Factory.h"

#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/CoreTypes.h"

#include "NodeGraph/Nodes/MathNodes.h"
#include "NodeGraph/Nodes/EntityNodes.h"
#include "NodeGraph/Nodes/TriggerNodes.h"
#include "NodeGraph/Nodes/DebugNodes.h"
#include "NodeGraph/FactoryHelper.h"

#include <choc/text/choc_StringUtilities.h>

namespace Shark::NodeGraph::Editor {

	ProcessNode* AbstractFactory::AllocateProcess(std::string_view category, std::string_view type, UUID id, NodeContext* context) const
	{
		const auto cat = m_Registry.find(category);
		if (cat == m_Registry.end())
			return nullptr;

		const auto entry = cat->second.find(type);
		if (entry == cat->second.end())
			return nullptr;

		return entry->second.ProcessAllocator(id, context);
	}

	bool AbstractFactory::SpawnNode(std::string_view category, std::string_view type, Node& outNode) const
	{
		const auto cat = m_Registry.find(category);
		if (cat != m_Registry.end())
		{
			const auto entry = cat->second.find(type);
			if (entry != cat->second.end())
			{
				entry->second.Spawner(category, outNode);
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

	template<typename TProcNode>
	static std::pair<std::string, FactoryEntry> CreateRegistryEntry(CoreFactory* factory, const NodeSettings& settings = {})
	{
		return {
			choc::text::replace(NodeType<TProcNode>::Inputs::Class, "<", " (", ">", ")"),
			{
				std::bind_front(FactoryHelper<CoreFactory>::ConstructNode<TProcNode>, factory, settings),
				[](UUID id, NodeContext* context) -> ProcessNode* { return new TProcNode(id, context); }
			}
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

	std::optional<int> CoreFactory::GetPinTypeOverride(std::string_view node, std::string_view pin) const
	{
		return std::nullopt;
	}

}
