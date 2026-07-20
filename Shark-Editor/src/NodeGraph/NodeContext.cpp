#include "skpch.h"
#include "NodeContext.h"

#include "Shark/Scene/Scene.h"

namespace Shark::NodeGraph {

	NodeContext::NodeContext(const NodeContextSpecification& specification)
	{
		m_Scene = specification.ActiveScene;
		m_Seed = specification.RandomSeed.value_or(m_RandomDevice());
	}

	Ref<Scene> NodeContext::GetActiveScene() const
	{
		return m_Scene;
	}

	std::random_device& NodeContext::GetRandomDevice()
	{
		return m_RandomDevice;
	}

	uint32_t NodeContext::GetSeed()
	{
		return m_Seed;
	}

}
