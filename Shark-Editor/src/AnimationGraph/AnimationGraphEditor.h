#pragma once

#include "Shark/Core/Base.h"
#include "NodeGraph/NodeGraphEditor.h"

namespace Shark::NodeGraph::Editor {

	class AnimationGraphEditor : public NodeGraphEditor
	{
	public:
		AnimationGraphEditor();
		~AnimationGraphEditor();

		virtual void OnInitialize() override;
		virtual void OnShutdown() override;

		virtual void OnCompileGraph() override;
		virtual void OnDrawGraphIO() override;

	};

}

namespace Shark {
	using NodeGraph::Editor::AnimationGraphEditor;
}
