#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"

#include "NodeGraph/NodeGraphContext.h"

namespace Shark::NodeGraph::Editor {

	class AnimationGraphContext : public NodeGraphContext
	{
	public:
		AnimationGraphContext();
		~AnimationGraphContext();

		virtual std::span<const std::string> GetInputTypes() const override { return m_InputTypes; }
		virtual const AbstractFactory* GetFactory() const override { return m_Factory.Raw(); }

		void SetSkeleton(AssetHandle skeletonSource) { m_SkeletonSource = skeletonSource; }
		AssetHandle GetSkeleton() const { return m_SkeletonSource; }

	protected:
		virtual       std::vector<Node>& GetNodesInternal()       override { return m_Nodes; }
		virtual const std::vector<Node>& GetNodesInternal() const override { return m_Nodes; }
		virtual       std::vector<Link>& GetLinksInternal()       override { return m_Links; }
		virtual const std::vector<Link>& GetLinksInternal() const override { return m_Links; }
		virtual       Properties& GetInputsInternal()       override { return m_InputProperties; }
		virtual const Properties& GetInputsInternal() const override { return m_InputProperties; }

		virtual AssetType GetPinAssetType(const Pin* pin) const override;

	private:
		std::vector<Node> m_Nodes;
		std::vector<Link> m_Links;
		Properties m_InputProperties;

		Scope<AbstractFactory> m_Factory;
		std::vector<std::string> m_InputTypes = { "Bool", "Int", "Float", "EntityID" };

		AssetHandle m_SkeletonSource;
	};

}
