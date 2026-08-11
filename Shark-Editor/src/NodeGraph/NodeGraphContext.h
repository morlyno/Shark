#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"
#include "Shark/NodeGraph/Properties.h"

#include <imgui_node_editor.h>
#include <span>

namespace Shark::NodeGraph::Editor {
	class AbstractFactory;

	struct Node;
	struct Link;
	struct Pin;
}

namespace Shark::NodeGraph::Editor {

	enum class QueryLinkResult
	{
		Ok,
		SamePin,
		SameNode,
		CausesLoop,
		IncompadiblePinType,
		IncompadiblePinKind,
		InvalidEndPin
	};

	class NodeGraphContext
	{
	public:
		using PinID = ax::NodeEditor::PinId;
		using NodeID = ax::NodeEditor::NodeId;
		using LinkID = ax::NodeEditor::LinkId;

	public:
		std::span<Node>       GetNodes();
		std::span<const Node> GetNodes() const;
		std::span<Link>       GetLinks();
		std::span<const Link> GetLinks() const;

		const Node* FindNode(NodeID id) const;
		const Link* FindLink(LinkID id) const;
		const Link* FindLink(PinID id) const;
		const Pin*  FindPin(PinID id) const;

		Node* FindNode(NodeID id);
		Link* FindLink(LinkID id);
		Link* FindLink(PinID id);
		Pin* FindPin(PinID id);

		bool IsPinLinked(const Pin* pin) const;
		bool IsInputNode(const Node* node, std::string_view propertyName) const;

		QueryLinkResult QueryCanLinkStatus(const Pin* startPin, const Pin* endPin) const;
		bool CanCreateLink(const Pin* startPin, const Pin* endPin) const;
		void CreateLink(const Pin* startPin, const Pin* endPin);
		void RemoveLink(LinkID linkID);
		void RemoveLinks(PinID id);

		Node* CreateNode(std::string_view category, std::string_view type);
		Node* CreateInputNode(std::string_view propertyName);
		void RemoveNode(NodeID nodeID);

		virtual bool SaveGraphState(const char* data, size_t size) = 0;
		virtual std::string_view LoadGraphState() = 0;

	public:
		Properties& GetInputs();
		const Properties& GetInputs() const;

		void AddInput(choc::value::Value value);
		void RemoveInput(std::string_view name);
		void ChangeInputType(std::string_view name, const choc::value::Type& newType);
		bool RenameInput(std::string_view oldName, std::string newName);

		virtual std::span<const std::string> GetInputTypes() const = 0;
		virtual AssetType GetPinAssetType(const Pin* pin) const = 0;

		virtual const AbstractFactory* GetFactory() const = 0;

	protected:
		virtual void OnInputRemoved(std::string_view name);
		virtual void OnInputTypeChanged(std::string_view name, const choc::value::Type& newType);
		virtual void OnInputRenamed(std::string_view oldName, std::string newName);

	protected:
		virtual       std::vector<Node>& GetNodesInternal()       = 0;
		virtual const std::vector<Node>& GetNodesInternal() const = 0;
		virtual       std::vector<Link>& GetLinksInternal()       = 0;
		virtual const std::vector<Link>& GetLinksInternal() const = 0;
		virtual       Properties& GetInputsInternal() = 0;
		virtual const Properties& GetInputsInternal() const = 0;
	};

}
