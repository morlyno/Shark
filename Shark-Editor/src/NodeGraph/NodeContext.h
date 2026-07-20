#pragma once

#include "Shark/Core/Base.h"
#include <random>

namespace Shark {
	class Scene;
}

namespace Shark::NodeGraph {

	struct NodeContextSpecification
	{
		Ref<Scene> ActiveScene;
		std::optional<uint32_t> RandomSeed;
	};

	class NodeContext
	{
	public:
		NodeContext(const NodeContextSpecification& specification);

		Ref<Scene>          GetActiveScene() const;
		std::random_device& GetRandomDevice();
		virtual uint32_t    GetSeed();

	private:
		Ref<Scene> m_Scene;
		std::random_device m_RandomDevice;

		uint32_t m_Seed;
	};

}
