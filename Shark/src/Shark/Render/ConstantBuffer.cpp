#include "skpch.h"
#include "ConstantBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

namespace Shark {

	Ref<ConstantBuffer> ConstantBuffer::Create(const ConstantBufferSpecification& specification, Buffer initData)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXConstantBuffer>::Create(specification, initData);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

	Ref<ConstantBuffer> ConstantBuffer::Create(BufferUsage usage, uint32_t byteSize, const std::string& debugName)
	{
		ConstantBufferSpecification specification;
		specification.ByteSize = byteSize;
		specification.Usage = usage;
		specification.DebugName = debugName;
		return Create(specification);
	}

}