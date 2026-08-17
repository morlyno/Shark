#include "skpch.h"
#include "AnimationGraphEditor.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/UI/Widgets.h"

#include "Shark/Animation/Graph/AnimationGraph.h"

#include "Shark/NodeGraph/Prototype.h"
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
		auto graphAsset = graphContext->GetGraphAsset();

		graphAsset->Prototype = graphContext->CreatePrototype();
		m_AnimationGraph = nullptr;
		graphContext->SaveGraph();
	}

	void AnimationGraphEditor::OnPlayGraph()
	{
		if (!m_AnimationGraph)
		{
			auto* graphContext = static_cast<AnimationGraphContext*>(GetGraphContext());
			auto graphAsset = graphContext->GetGraphAsset();

			if (!graphAsset->Prototype)
				return;

			m_AnimationGraph = graphAsset->CreateGraph();

			m_AnimationGraph->InitializeGraph();
		}

		m_AnimationGraph->Process(1.0f / 60.0f);
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
