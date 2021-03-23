#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {
	
	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, const std::string& filepath)
	{
		return RendererCommand::CreateTexture2D(specs, filepath);
	}

	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t flatcolor)
	{
		return RendererCommand::CreateTexture2D(specs, width, height, flatcolor);
	}

	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, uint32_t width, uint32_t height, void* data)
	{
		return RendererCommand::CreateTexture2D(specs, width, height, data);
	}
}