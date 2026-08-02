#pragma once

#include "Shark/Core/Base.h"

#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {
	struct ProcessNode;
	class NodeContext;

	namespace Editor {
		struct Node;
		struct Pin;
		struct NodeSettings;
	}
}

namespace Shark::NodeGraph::Editor {

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Factory Entry /////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	struct FactoryEntry
	{
		using SpawnerSignature = void(std::string_view category, Node& outNode);
		using ProcessNodeAllocator = ProcessNode * (UUID id, NodeContext*);

		std::function<SpawnerSignature> Spawner;
		std::function<ProcessNodeAllocator> ProcessAllocator;
	};

	using FactoryRegistry = std::map<std::string, std::map<std::string, FactoryEntry, std::ranges::less>, std::ranges::less>;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Abstract Factory //////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class AbstractFactory
	{
	public:
		virtual bool InitializePin(Pin& outPin, int pinType) const = 0;
		virtual bool InitializePin(Pin& outPin, const std::type_info& type) const = 0;
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const = 0;
		virtual std::optional<int> GetPinTypeOverride(std::string_view node, std::string_view pin) const = 0;

		ProcessNode* AllocateProcess(std::string_view category, std::string_view type, UUID id, NodeContext* context) const;
		bool         SpawnNode(std::string_view category, std::string_view type, Node& outNode) const;
		
		const FactoryRegistry& GetRegistry() const { return m_Registry; }
		void Merge(FactoryRegistry registry);

	private:
		FactoryRegistry m_Registry;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Core Factory //////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class CoreFactory : public AbstractFactory
	{
	public:
		CoreFactory();

		virtual bool InitializePin(Pin& outPin, int pinType) const override;
		virtual bool InitializePin(Pin& outPin, const std::type_info& type) const override;
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const override;
		virtual std::optional<int> GetPinTypeOverride(std::string_view node, std::string_view pin) const override;
	};

}
