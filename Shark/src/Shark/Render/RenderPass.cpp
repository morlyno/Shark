#include "skpch.h"
#include "RenderPass.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXRenderPass.h"

namespace Shark {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& specification)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXRenderPass>::Create(specification);
		}
#
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

}
