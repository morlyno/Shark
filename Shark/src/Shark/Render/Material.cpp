#include "skpch.h"
#include "Material.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXMaterial.h"

namespace Shark {

	Ref<Material> Material::Create(const Ref<Shaders>& shaders, const std::string& name)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specifies"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXMaterial>::Create(shaders, name);
		}
		SK_CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}

}