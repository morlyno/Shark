#include "skpch.h"
#include "RenderCommandBuffer.h"

#include "Shark/Render/RendererAPI.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

namespace Shark {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXRenderCommandBuffer>::Create();
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(Ref<RenderCommandBuffer> parentCommandBuffer)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXRenderCommandBuffer>::Create(parentCommandBuffer);
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

}