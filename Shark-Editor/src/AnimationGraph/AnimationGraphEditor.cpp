#include "skpch.h"
#include "AnimationGraphEditor.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"

#include "Shark/UI/Widgets.h"

#include "AnimationGraph/AnimationGraphContext.h"
#include "AnimationGraph/AnimationNodeContext.h"
#include "AnimationGraph/AnimationFactory.h"

namespace Shark::NodeGraph::Editor {

	AnimationGraphEditor::AnimationGraphEditor()
	{
	}

	AnimationGraphEditor::~AnimationGraphEditor()
	{
	}

	void AnimationGraphEditor::OnInitialize()
	{
		SetGraphContext(Scope<AnimationGraphContext>::Create());
	}

	void AnimationGraphEditor::OnShutdown()
	{
	}

	void AnimationGraphEditor::OnCompileGraph()
	{
		auto* graphContext = static_cast<AnimationGraphContext*>(GetGraphContext());

		auto meshSource = AssetManager::GetAsset<MeshSource>(graphContext->GetSkeleton());
		const Skeleton* skeleton = &meshSource->GetSkeleton();

		AnimationContextSpecification specification;
		SetupNodeContext(specification);
		specification.BoneCount = skeleton->GetBoneCount();
		specification.Skeleton = skeleton;

		AnimationNodeContext context(specification);
		m_NodeGraph = CompileGraph(&context);
	}

	void AnimationGraphEditor::OnDrawGraphIO()
	{
		auto* context = static_cast<AnimationGraphContext*>(GetGraphContext());

		ImGui::BeginHorizontal("##graphIO", { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() });
		ImGui::Text("Skeleton");
		ImGui::Spring();
		AssetHandle skeleton = context->GetSkeleton();
		if (UI::Widgets::SelectAsset(AssetType::MeshSource, skeleton))
		{
			context->SetSkeleton(skeleton);
		}
		ImGui::EndHorizontal();
	}

}
