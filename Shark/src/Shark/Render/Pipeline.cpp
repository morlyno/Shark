#include "skpch.h"
#include "Pipeline.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXPipeline.h"

namespace Shark {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI Spcified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXPipeline>::Create(specs);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}
