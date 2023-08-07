#include "skpch.h"
#include "ConstantBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

namespace Shark {

	Ref<ConstantBuffer> ConstantBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXConstantBuffer>::Create(size, binding);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}