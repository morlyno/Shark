#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Animation/Graph/AnimationGraphAsset.h"

#include "Shark/NodeGraph/Properties.h"

namespace Shark {
	class StreamWriter;

	namespace NodeGraph::Editor {
		struct Node;
		struct Link;
	}
}

namespace Shark {

	class EditorAnimationGraphAsset : public AnimationGraphAsset
	{
	public:
		EditorAnimationGraphAsset();
		~EditorAnimationGraphAsset();

		      std::vector<NodeGraph::Editor::Node>& GetNodes();
		const std::vector<NodeGraph::Editor::Node>& GetNodes() const;
		      std::vector<NodeGraph::Editor::Link>& GetLinks();
		const std::vector<NodeGraph::Editor::Link>& GetLinks() const;

		NodeGraph::Properties& GetProperties();
		const NodeGraph::Properties& GetProperties() const;

		      std::string& GetGraphState()       { return m_GraphState; };
		const std::string& GetGraphState() const { return m_GraphState; };

	private:
		std::vector<NodeGraph::Editor::Node> m_Nodes;
		std::vector<NodeGraph::Editor::Link> m_Links;
		NodeGraph::Properties m_InputProperties;

		std::string m_GraphState;

		friend class EditorAnimationGraphSerializer;
	};

}
