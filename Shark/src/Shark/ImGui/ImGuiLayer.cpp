#include "skpch.h"
#include "ImGuiLayer.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXImGuiLayer.h"

namespace Shark {

	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI specified"); return nullptr;
			case RendererAPI::API::DirectX11: return (ImGuiLayer*)new DirectXImGuiLayer();
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}

