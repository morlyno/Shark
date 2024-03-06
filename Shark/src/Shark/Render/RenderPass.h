#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/Pipeline.h"
#include "Shark/Render/ConstantBuffer.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/RendererResource.h"

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
		virtual void Update() = 0;

		virtual void Set(const std::string& name, Ref<ConstantBuffer> constantBuffer) = 0;
		virtual void Set(const std::string& name, Ref<StorageBuffer> storageBuffer) = 0;
		virtual void Set(const std::string& name, Ref<Image2D> image) = 0;
		virtual void Set(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void Set(const std::string& name, Ref<TextureCube> textureCube) = 0;

		virtual Ref<ConstantBuffer> GetConstantBuffer(const std::string& name) const = 0;
		virtual Ref<StorageBuffer> GetStorageBuffer(const std::string& name) const = 0;
		virtual Ref<Image2D> GetImage2D(const std::string& name) const = 0;
		virtual Ref<Texture2D> GetTexture2D(const std::string& name) const = 0;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) const = 0;

		virtual Ref<RendererResource> GetInput(const std::string& name) const = 0;

		template<typename T>
		Ref<T> GetInput(const std::string& name) const
		{
			return GetInput(name).As<T>();
		}

		virtual Ref<Image2D> GetOutput(uint32_t index) const = 0;
		virtual Ref<Image2D> GetDepthOutput() const = 0;

		virtual Ref<Pipeline> GetPipeline() const = 0;
		virtual RenderPassSpecification& GetSpecification() = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;

	public:
		static Ref<RenderPass> Create(const RenderPassSpecification& specification);

	};

}
