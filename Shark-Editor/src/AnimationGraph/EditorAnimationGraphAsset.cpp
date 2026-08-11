#include "skpch.h"
#include "EditorAnimationGraphAsset.h"

#include "NodeGraph/EditorNodes.h"

namespace NG = ::Shark::NodeGraph;
namespace NGE = ::Shark::NodeGraph::Editor;

namespace Shark {

	EditorAnimationGraphAsset::EditorAnimationGraphAsset()
	{

	}

	EditorAnimationGraphAsset::~EditorAnimationGraphAsset()
	{

	}

	std::vector<NGE::Node>& EditorAnimationGraphAsset::GetNodes()
	{
		return m_Nodes;
	}

	const std::vector<NGE::Node>& EditorAnimationGraphAsset::GetNodes() const
	{
		return m_Nodes;
	}

	std::vector<NGE::Link>& EditorAnimationGraphAsset::GetLinks()
	{
		return m_Links;
	}

	const std::vector<NGE::Link>& EditorAnimationGraphAsset::GetLinks() const
	{
		return m_Links;
	}

	NG::Properties& EditorAnimationGraphAsset::GetProperties()
	{
		return m_InputProperties;
	}

	const NG::Properties& EditorAnimationGraphAsset::GetProperties() const
	{
		return m_InputProperties;
	}

}
