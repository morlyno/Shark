#pragma once

#include "Shark/Render/RenderPass.h"
#include "Platform/DirectX11/ShaderInputManager.h"

namespace Shark {

	class DirectXRenderPass : public RenderPass
	{
	public:
		DirectXRenderPass(const RenderPassSpecification& specification);
		~DirectXRenderPass();

		virtual void Bake() override;
		virtual bool Validate() const override;
		virtual void Update() override;

		virtual void Set(const std::string& name, Ref<ConstantBuffer> constantBuffer) override;
		virtual void Set(const std::string& name, Ref<StorageBuffer> storageBuffer) override;
		virtual void Set(const std::string& name, Ref<Image2D> image) override;
		virtual void Set(const std::string& name, Ref<Texture2D> texture) override;
		virtual void Set(const std::string& name, Ref<TextureCube> textureCube) override;

		virtual Ref<ConstantBuffer> GetConstantBuffer(const std::string& name) const override;
		virtual Ref<StorageBuffer> GetStorageBuffer(const std::string& name) const override;
		virtual Ref<Image2D> GetImage2D(const std::string& name) const override;
		virtual Ref<Texture2D> GetTexture2D(const std::string& name) const override;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) const override;

		virtual Ref<RendererResource> GetInput(const std::string& name) const override;

		virtual Ref<Image2D> GetOutput(uint32_t index) const override;
		virtual Ref<Image2D> GetDepthOutput() const override;

		virtual Ref<Pipeline> GetPipeline() const override { return m_Specification.Pipeline; }
		virtual RenderPassSpecification& GetSpecification() { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

		const std::vector<BoundResource>& GetBoundResources() const { return m_ShaderInputManager.GetBoundResources(); }

	private:
		RenderPassSpecification m_Specification;
		ShaderInputManager m_ShaderInputManager;

		friend class DirectXRenderer;
	};

}

