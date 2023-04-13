#include "skpch.h"
#include "ImGuiLayer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXImGuiLayer.h"

namespace Shark {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI specified"); return nullptr;
			case RendererAPIType::DirectX11: return (ImGuiLayer*)sknew DirectXImGuiLayer();
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}

