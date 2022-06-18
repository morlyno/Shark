#include "skpch.h"
#include "RenderCommandBuffer.h"

#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

namespace Shark {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXRenderCommandBuffer>::Create();
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

}