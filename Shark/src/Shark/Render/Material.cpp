#include "skpch.h"
#include "Material.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXMaterial.h"

namespace Shark {

	Ref<Material> Material::Create(Ref<Shader> shader)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXMaterial>::Create(shader);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}
