#include "skpch.h"
#include "RendererContext.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

namespace Shark {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXContext>::Create();
		}
		SK_CORE_VERIFY(false, "Unkown RendererAPI");
		return nullptr;
	}

}
