#include "skpch.h"
#include "Material.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXMaterial.h"

namespace Shark {

	Ref<Material> Material::Create(Ref<Shader> shader, const std::string& name)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXMaterial>::Create(shader, name);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}
