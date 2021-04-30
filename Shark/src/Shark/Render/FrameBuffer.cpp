#include "skpch.h"
#include "FrameBuffer.h"

#include "Platform/DirectX11/DirectXFrameBuffer.h"

namespace Shark {

    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification& specs)
    {
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXFrameBuffer>::Create(specs);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
    }

}
