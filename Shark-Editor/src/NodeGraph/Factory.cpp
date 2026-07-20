#include "skpch.h"
#include "Factory.h"

#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/CoreTypes.h"

namespace Shark::NodeGraph::Editor {

	CoreFactory::CoreFactory(Registry registry)
		: m_Registry(std::move(registry))
	{
	}

	ProcessNode* CoreFactory::AllocateProcess(std::string_view category, std::string_view type, UUID id) const
	{
		const auto cat = m_Registry.find(category);
		if (cat == m_Registry.end())
			return nullptr;

		const auto entry = cat->second.find(type);
		if (entry == cat->second.end())
			return nullptr;

		return entry->second.ProcessAllocator(id);
	}

	bool CoreFactory::CoreSpawnNode(std::string_view category, std::string_view type, Node& outNode, UUID id) const
	{
		const auto cat = m_Registry.find(category);
		if (cat != m_Registry.end())
		{
			const auto entry = cat->second.find(type);
			if (entry != cat->second.end())
			{
				entry->second.Spawner(category, outNode, id);
				return true;
			}
		}
		return false;
	}

	bool CoreFactory::CoreInitializePin(Pin& outPin, int pinType) const
	{
		return CoreTypes::InitializePin(outPin, static_cast<CoreTypes::EPinType>(pinType));
	}

	bool CoreFactory::CoreInitializePin(Pin& outPin, const choc::value::Type& type) const
	{
		return CoreTypes::InitiailizePin(outPin, type);
	}

}
