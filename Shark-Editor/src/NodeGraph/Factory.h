#pragma once

#include "Shark/Core/Base.h"

#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {
	struct ProcessNode;

	namespace Editor {
		struct Node;
		struct Pin;
	}
}

namespace Shark::NodeGraph::Editor {

	class AbstractFactory
	{
	public:
		using SpawnerSignature = void(std::string_view category, Node& outNode, UUID id);
		using ProcessNodeAllocator = ProcessNode*(UUID id);

		struct Entry
		{
			std::function<SpawnerSignature> Spawner;
			std::function<ProcessNodeAllocator> ProcessAllocator;
		};

		using Registry = std::map<std::string, std::map<std::string, Entry, std::ranges::less>, std::ranges::less>;

		virtual ProcessNode* AllocateProcess(std::string_view category, std::string_view type, UUID id) const = 0;
		virtual bool SpawnNode(std::string_view category, std::string_view type, Node& outNode, UUID id = UUID::Generate()) const = 0;
		virtual bool InitializePin(Pin& outPin, int pinType) const = 0;
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const = 0;
		virtual auto GetRegistry() const -> const Registry& = 0;
	};

	class CoreFactory : public AbstractFactory
	{
	public:
		CoreFactory() = default;
		CoreFactory(Registry registry);

		virtual ProcessNode* AllocateProcess(std::string_view category, std::string_view type, UUID id) const override;
		virtual bool SpawnNode(std::string_view category, std::string_view type, Node& outNode, UUID id) const override { return CoreSpawnNode(category, type, outNode, id); }
		virtual bool InitializePin(Pin& outPin, int pinType) const override                                             { return CoreInitializePin(outPin, pinType); }
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const override                           { return CoreInitializePin(outPin, type); }
		virtual const Registry& GetRegistry() const override                                                            { return m_Registry; }
		Registry& GetRegistry()																							{ return m_Registry; }

		bool CoreSpawnNode(std::string_view category, std::string_view type, Node& outNode, UUID id) const;
		bool CoreInitializePin(Pin& outPin, int pinType) const;
		bool CoreInitializePin(Pin& outPin, const choc::value::Type& type) const;

	private:
		Registry m_Registry;
	};

}
