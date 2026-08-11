#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"

#include "NodeGraph/NodeGraphContext.h"

namespace Shark {
	class EditorAnimationGraphAsset;
}

namespace Shark::NodeGraph::Editor {

	class AnimationGraphContext : public NodeGraphContext
	{
	public:
		AnimationGraphContext(Ref<EditorAnimationGraphAsset> animationGraph);
		~AnimationGraphContext();

		virtual std::span<const std::string> GetInputTypes() const override { return m_InputTypes; }
		virtual const AbstractFactory* GetFactory() const override { return m_Factory.Raw(); }

		void SetSkeleton(AssetHandle skeletonSource);
		AssetHandle GetSkeleton() const;

		virtual bool SaveGraphState(const char* data, size_t size) override;
		virtual std::string_view LoadGraphState() override;
		void SaveGraph() const;

	protected:
		virtual       std::vector<Node>& GetNodesInternal()       override;
		virtual const std::vector<Node>& GetNodesInternal() const override;
		virtual       std::vector<Link>& GetLinksInternal()       override;
		virtual const std::vector<Link>& GetLinksInternal() const override;
		virtual       Properties& GetInputsInternal()       override;
		virtual const Properties& GetInputsInternal() const override;

		virtual AssetType GetPinAssetType(const Pin* pin) const override;

	private:
		Ref<EditorAnimationGraphAsset> m_AnimationGraph;

		Scope<AbstractFactory> m_Factory;
		std::vector<std::string> m_InputTypes = { "Bool", "Int", "Float", "EntityID" };
	};

}
