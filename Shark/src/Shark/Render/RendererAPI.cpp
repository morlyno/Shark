#include "skpch.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::DirectX11;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
			case API::None:        SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case API::DirectX11:   return CreateScope<DirectXRendererAPI>();
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}