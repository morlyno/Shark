#include "skpch.h"
#include "AnimationGraphContext.h"

#include "Shark/Core/Project.h"

#include "NodeGraph/EditorNodes.h"
#include "AnimationGraph/EditorAnimationGraphAsset.h"
#include "AnimationGraph/AnimationFactory.h"

namespace Shark::NodeGraph::Editor {

	AnimationGraphContext::AnimationGraphContext(Ref<EditorAnimationGraphAsset> animationGraph)
		: m_AnimationGraph(animationGraph), m_Factory(Scope<AnimationFactory>::Create())
	{
	}

	AnimationGraphContext::~AnimationGraphContext()
	{
	}

	void AnimationGraphContext::SetSkeleton(AssetHandle skeletonSource)
	{
		m_AnimationGraph->SetSkeletonMesh(skeletonSource);
	}

	AssetHandle AnimationGraphContext::GetSkeleton() const
	{
		return m_AnimationGraph->GetSkeletonMesh();
	}

	bool AnimationGraphContext::SaveGraphState(const char* data, size_t size)
	{
		auto& state = m_AnimationGraph->GetGraphState();
		state.assign(data, size);
		return true;
	}

	std::string_view AnimationGraphContext::LoadGraphState()
	{
		return m_AnimationGraph->GetGraphState();
	}

	void AnimationGraphContext::SaveGraph() const
	{
		Project::GetEditorAssetManager()->SaveAsset(m_AnimationGraph->Handle);
	}

	std::vector<Node>& AnimationGraphContext::GetNodesInternal()
	{
		return m_AnimationGraph->GetNodes();
	}

	const std::vector<Node>& AnimationGraphContext::GetNodesInternal() const
	{
		return m_AnimationGraph->GetNodes();
	}

	std::vector<Link>& AnimationGraphContext::GetLinksInternal()
	{
		return m_AnimationGraph->GetLinks();
	}

	const std::vector<Link>& AnimationGraphContext::GetLinksInternal() const
	{
		return m_AnimationGraph->GetLinks();
	}

	Properties& AnimationGraphContext::GetInputsInternal()
	{
		return m_AnimationGraph->GetProperties();
	}

	const Properties& AnimationGraphContext::GetInputsInternal() const
	{
		return m_AnimationGraph->GetProperties();
	}

	AssetType AnimationGraphContext::GetPinAssetType(const Pin* pin) const
	{
		const Node* node = FindNode(pin->NodeID);

		if (node->Name == "AnimationPlayer" && pin->Name == "Animation")
			return AssetType::Animation;

		return AssetType::None;
	}

}
