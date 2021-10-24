#include "skpch.h"
#include "ConstantBuffer.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

namespace Shark {

	Ref<ConstantBuffer> ConstantBuffer::Create(uint32_t size, uint32_t slot)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXConstantBuffer>::Create(size, slot);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

	Ref<ConstantBufferSet> ConstantBufferSet::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXConstantBufferSet>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}