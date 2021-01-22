#include "skpch.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	Ref<Texture2D> Texture2D::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXTexture2D>(filepath);
		}

		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}