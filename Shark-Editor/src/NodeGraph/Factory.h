#pragma once

#include "Shark/Core/Base.h"

#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {
	struct ProcessNode;
	struct NodeContext;

	namespace Editor {
		struct Node;
		struct Pin;
		struct NodeSettings;
	}
}

namespace Shark::NodeGraph::Editor {

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Factory Registry //////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	using NodeSpawnerSignature = void(std::string_view category, Node& outNode);
	using FactoryRegistry = std::map<std::string, std::map<std::string, std::function<NodeSpawnerSignature>, std::ranges::less>, std::ranges::less>;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Abstract Factory //////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class AbstractFactory
	{
	public:
		virtual ~AbstractFactory() = default;
		virtual bool InitializePin(Pin& outPin, int pinType) const = 0;
		virtual bool InitializePin(Pin& outPin, const std::type_info& type) const = 0;
		virtual bool InitializePin(Pin& outPin, const choc::value::Type& type) const = 0;
		virtual choc::value::Type GetTypeFromPinType(int pinType) const = 0;
		virtual std::optional<int> GetPinTypeOverride(std::string_view node, std::string_view pin) const = 0;

		Pin ConstructPin(std::string_view name, int pinType) const;
		bool SpawnNode(std::string_view category, std::string_view type, Node& outNode) const;
		
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
		virtual choc::value::Type GetTypeFromPinType(int pinType) const override;
		virtual std::optional<int> GetPinTypeOverride(std::string_view node, std::string_view pin) const override;
	};

}
