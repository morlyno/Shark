#include "skpch.h"
#include "StorageBuffer.h"

#include "Shark/Render/RendererAPI.h"

#include "Platform/DirectX11/DirectXStorageBuffer.h"

namespace Shark {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t structSize, uint32_t count)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXStorageBuffer>::Create(structSize, count);
		}
		SK_CORE_VERIFY(false, "Unkown RendererAPIType");
		return nullptr;
	}

}
