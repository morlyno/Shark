#include "skpch.h"
#include "AnimationGraphEditor.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/UI/Widgets.h"

#include "Shark/Animation/Graph/AnimationNodeContext.h"

#include "AnimationGraph/AnimationGraphContext.h"
#include "AnimationGraph/EditorAnimationGraphAsset.h"

namespace Shark::NodeGraph::Editor {

	AnimationGraphEditor::AnimationGraphEditor(const std::string& name, const AssetMetaData& metadata)
		: NodeGraphEditor(name, metadata)
	{
		auto graph = AssetManager::GetAsset<EditorAnimationGraphAsset>(metadata.Handle);
		SetGraphContext(Scope<AnimationGraphContext>::Create(graph));
	}

	AnimationGraphEditor::~AnimationGraphEditor()
	{
	}

	void AnimationGraphEditor::OnCompileGraph()
	{
		auto* graphContext = static_cast<AnimationGraphContext*>(GetGraphContext());

		auto meshSource = AssetManager::GetAsset<MeshSource>(graphContext->GetSkeleton());
		const Skeleton* skeleton = &meshSource->GetSkeleton();

		AnimationContextSpecification specification;
		SetupNodeContext(specification);
		specification.BoneCount = static_cast<uint32_t>(skeleton->GetBoneCount());
		specification.Skeleton = skeleton;

		AnimationNodeContext context(specification);
		m_NodeGraph = CompileGraph(&context);

		graphContext->SaveGraph();
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
