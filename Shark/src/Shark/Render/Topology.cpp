#include "skpch.h"
#include "Topology.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXTopology.h"

namespace Shark {

	Ref<Topology> Topology::Create(TopologyMode topology)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTopology>::Allocate(topology);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}