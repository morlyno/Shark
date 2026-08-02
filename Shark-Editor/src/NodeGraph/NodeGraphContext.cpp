#include "skpch.h"
#include "NodeGraphContext.h"

#include "NodeGraph/Factory.h"

namespace Shark::NodeGraph::Editor {

	namespace utils {

		static uint64_t NextID()
		{
			return UUID::Generate().Value();
		}

	}

	std::span<Node> NodeGraphContext::GetNodes()
	{
		return GetNodesInternal();
	}

	std::span<const Node> NodeGraphContext::GetNodes() const
	{
		return GetNodesInternal();
	}

	std::span<Link> NodeGraphContext::GetLinks()
	{
		return GetLinksInternal();
	}

	std::span<const Link> NodeGraphContext::GetLinks() const
	{
		return GetLinksInternal();
	}

	const Node* NodeGraphContext::FindNode(NodeID id) const
	{
		for (auto& node : GetNodesInternal())
			if (node.ID == id)
				return &node;

		return nullptr;
	}

	const Link* NodeGraphContext::FindLink(LinkID id) const
	{
		for (auto& link : GetLinksInternal())
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	const Link* NodeGraphContext::FindLink(PinID id) const
	{
		for (auto& link : GetLinksInternal())
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	const Pin* NodeGraphContext::FindPin(PinID id) const
	{
		if (!id)
			return nullptr;

		for (auto& node : GetNodesInternal())
		{
			for (auto& pin : node.Inputs)
				if (pin.ID == id)
					return &pin;

			for (auto& pin : node.Outputs)
				if (pin.ID == id)
					return &pin;
		}

		return nullptr;
	}

	bool NodeGraphContext::IsPinLinked(const Pin* pin) const
	{
		for (auto& link : GetLinksInternal())
			if (link.StartPinID == pin->ID || link.EndPinID == pin->ID)
				return true;

		return false;
	}

	bool NodeGraphContext::IsInputNode(const Node* node, std::string_view propertyName) const
	{
		return node->Outputs.size() == 1 && node->Name == "Input" && node->Outputs[0].Name == propertyName;
	}

	QueryLinkResult NodeGraphContext::QueryCanLinkStatus(const Pin* startPin, const Pin* endPin) const
	{
		if (startPin == endPin)
			return QueryLinkResult::SamePin;

		if (startPin->NodeID == endPin->NodeID)
			return QueryLinkResult::SameNode;

		if (startPin->Kind == endPin->Kind)
			return QueryLinkResult::IncompadiblePinKind;

		if (startPin->PinType != endPin->PinType)
			return QueryLinkResult::IncompadiblePinType;

		if (startPin->Kind == ax::NodeEditor::PinKind::Input)
			std::swap(startPin, endPin);

		if (IsPinLinked(endPin))
			return QueryLinkResult::InvalidEndPin;

		auto wouldCreateLoop = [this](auto& call, const Pin* startPin, const Pin* endPin) -> bool
		{
			if (startPin->NodeID == endPin->NodeID)
				return true;

			auto* startNode = FindNode(startPin->NodeID);
			for (auto& inputPin : startNode->Inputs)
			{
				auto* link = FindLink(inputPin.ID);
				if (!link)
					continue;

				auto* pin = FindPin(link->StartPinID);
				SK_CORE_ASSERT(pin, "Should never be null");
				if (call(call, pin, endPin))
					return true;
			}

			return false;
		};

		if (wouldCreateLoop(wouldCreateLoop, startPin, endPin))
			return QueryLinkResult::CausesLoop;

		return QueryLinkResult::Ok;
	}

	bool NodeGraphContext::CanCreateLink(const Pin* startPin, const Pin* endPin) const
	{
		return QueryCanLinkStatus(startPin, endPin) == QueryLinkResult::Ok;
	}

	void NodeGraphContext::CreateLink(const Pin* startPin, const Pin* endPin)
	{
		if (startPin->Kind == ax::NodeEditor::PinKind::Input)
			std::swap(startPin, endPin);

		GetLinksInternal().push_back(
			Link{
				.ID = utils::NextID(),
				.StartPinID = startPin->ID,
				.EndPinID = endPin->ID,
				.Color = startPin->Color
			}
		);
	}

	void NodeGraphContext::RemoveLink(LinkID linkID)
	{
		std::erase_if(GetLinksInternal(), [linkID](const Link& link) { return link.ID == linkID; });
	}

	void NodeGraphContext::RemoveLinks(PinID id)
	{
		auto& links = GetLinksInternal();

		for (auto link = links.begin(); link != links.end();)
		{
			if (link->StartPinID != id && link->EndPinID != id)
			{
				++link;
				continue;
			}

			link = links.erase(link);
		}
	}

	Node* NodeGraphContext::CreateNode(std::string_view category, std::string_view type)
	{
		auto& node = GetNodesInternal().emplace_back();
		GetFactory()->SpawnNode(category, type, node);

		return &node;
	}

	Node* NodeGraphContext::CreateInputNode(std::string_view propertyName)
	{
		auto& properties = GetInputsInternal();
		if (!properties.HasValue(propertyName))
			return nullptr;

		auto value = properties.GetValue(propertyName);

		auto& node = GetNodesInternal().emplace_back();
		node.ID = utils::NextID();
		node.Name = "Input";
		node.Category = "Property";

		auto& pin = node.Outputs.emplace_back();
		GetFactory()->InitializePin(pin, value.getType());
		pin.ID = utils::NextID();
		pin.NodeID = node.ID;
		pin.Kind = ax::NodeEditor::PinKind::Output;
		pin.Name = std::string(propertyName);

		return &node;
	}

	void NodeGraphContext::RemoveNode(NodeID nodeID)
	{
		auto& nodes = GetNodesInternal();
		for (size_t i = 0; i < nodes.size(); i++)
		{
			auto& node = nodes[i];
			if (node.ID != nodeID)
				continue;

			for (auto& pin : node.Inputs)
				RemoveLinks(pin.ID);

			for (auto& pin : node.Outputs)
				RemoveLinks(pin.ID);

			nodes.erase(nodes.begin() + i);
			break;
		}
	}

	Properties& NodeGraphContext::GetInputs()
	{
		return GetInputsInternal();
	}

	const Properties& NodeGraphContext::GetInputs() const
	{
		return GetInputsInternal();
	}

	void NodeGraphContext::AddInput(choc::value::Value value)
	{
		auto& properties = GetInputsInternal();
		std::string name = "New Input";

		size_t index = 0;
		while (properties.HasValue(name))
			name = fmt::format("New Input ({})", index++);

		properties.Set(name, std::move(value));
	}

	void NodeGraphContext::RemoveInput(std::string_view name)
	{
		auto& properties = GetInputsInternal();
		properties.Remove(name);

		OnInputRemoved(name);
	}

	void NodeGraphContext::ChangeInputType(std::string_view name, const choc::value::Type& newType)
	{
		auto& properties = GetInputsInternal();
		if (!properties.HasValue(name))
			return;

		auto value = properties.GetValue(name);
		if (value.getType() != newType)
		{
			properties.Set(name, choc::value::Value(newType));

			OnInputTypeChanged(name, newType);
		}
	}

	bool NodeGraphContext::RenameInput(std::string_view oldName, std::string newName)
	{
		auto& properties = GetInputsInternal();
		if (!properties.HasValue(oldName) || properties.HasValue(newName))
			return false;

		auto value = choc::value::Value(properties.GetValue(oldName));
		properties.Remove(oldName);
		properties.Set(newName, std::move(value));
		OnInputRenamed(oldName, newName);
		return true;
	}

	void NodeGraphContext::OnInputRemoved(std::string_view name)
	{
		auto range = std::ranges::remove_if(GetNodesInternal(), [this, name](auto& node)
		{
			return IsInputNode(&node, name);
		});

		for (auto& node : range | std::views::reverse)
		{
			RemoveNode(node.ID);
		}
	}

	void NodeGraphContext::OnInputTypeChanged(std::string_view name, const choc::value::Type& newType)
	{
		for (auto& node : GetNodesInternal())
		{
			if (!IsInputNode(&node, name))
				continue;

			for (auto& output : node.Outputs)
			{
				RemoveLinks(output.ID);

				GetFactory()->InitializePin(output, newType);
				output.Value = choc::value::Value(newType);
			}
		}
	}

	void NodeGraphContext::OnInputRenamed(std::string_view oldName, std::string newName)
	{
		for (auto& node : GetNodesInternal())
		{
			if (!IsInputNode(&node, oldName))
				continue;

			node.Outputs[0].Name = newName;;
		}
	}

}
