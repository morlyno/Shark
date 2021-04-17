#include "skpch.h"
#include "Rasterizer.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXRasterizer.h"

namespace Shark {

	Ref<Rasterizer> Rasterizer::Create(const RasterizerSpecification& specs)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXRasterizer>::Allocate(specs);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}


}