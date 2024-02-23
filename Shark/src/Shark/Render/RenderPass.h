#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/Pipeline.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	struct RenderPassSpecification
	{
		Ref<Pipeline> Pipeline;
		std::string DebugName;
	};

	class RenderPass : public RefCount
	{
	public:
		virtual void Bake() = 0;
		virtual bool Validate() const = 0;

		virtual void Set(const std::string& name, Ref<ConstantBuffer> constantBuffer) = 0;
		virtual void Set(const std::string& name, Ref<Image2D> image) = 0;
		virtual void Set(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void Set(const std::string& name, Ref<TextureCube> textureCube) = 0;
		virtual void Set(const std::string& name, Ref<SamplerWrapper> sampler) = 0;

		virtual Ref<Image2D> GetOutput(uint32_t index) const = 0;
		virtual Ref<Image2D> GetDepthOutput() const = 0;

		virtual Ref<Pipeline> GetPipeline() const = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;

	public:
		static Ref<RenderPass> Create(const RenderPassSpecification& specification);

	};

}
