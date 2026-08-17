#pragma once

#include "Shark/Core/Base.h"
#include "NodeGraph/NodeGraphEditor.h"

namespace Shark::NodeGraph {
	class AnimationGraph;
}

namespace Shark::NodeGraph::Editor {

	class AnimationGraphEditor : public NodeGraphEditor
	{
	public:
		AnimationGraphEditor(const std::string& name, const AssetMetaData& metadata);
		~AnimationGraphEditor();

		virtual void OnCompileGraph() override;
		virtual void OnPlayGraph() override;
		virtual void OnDrawGraphIO() override;

	private:
		Ref<AnimationGraph> m_AnimationGraph;
	};

}

namespace Shark {
	using NodeGraph::Editor::AnimationGraphEditor;
}
