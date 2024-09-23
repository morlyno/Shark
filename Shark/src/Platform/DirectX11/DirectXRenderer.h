#pragma once

#include "Shark/Render/RendererAPI.h"

#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXMaterial.h"
#include "Platform/DirectX11/DirectXPipeline.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

#include <set>
#include <d3d11_1.h>

namespace Shark {

	class DirectXRenderer : public RendererAPI
	{
	public:
		DirectXRenderer();
		virtual ~DirectXRenderer();

		virtual void Init() override;
		virtual void ShutDown() override;

		virtual void BeginEventMarker(Ref<RenderCommandBuffer> commandBuffer, const std::string& name) override;
		virtual void EndEventMarker(Ref<RenderCommandBuffer> commandBuffer) override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, bool expliciteClear) override;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass) override;

		virtual void RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) override;

		virtual void BeginBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) override;
		virtual void RenderBatch(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, uint32_t indexCount, uint32_t startIndex) override;
		virtual void EndBatch(Ref<RenderCommandBuffer> renderCommandBuffer) override;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount) override;

		virtual void RenderCube(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material) override;

		virtual void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable) override;
		virtual void RenderSubmeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material) override;

		virtual void CopyImage(Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) override;
		virtual void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) override;
		virtual void BlitImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage) override;

		void RT_EquirectangularToCubemap(Ref<DirectXTexture2D> equirect, Ref<DirectXTextureCube> unfiltered, uint32_t cubemapSize);
		void RT_EnvironmentMipFilter(Ref<DirectXTextureCube> unfiltered, Ref<DirectXTextureCube> filtered, uint32_t cubemapSize);
		void RT_EnvironmentIrradiance(Ref<DirectXTextureCube> filtered, Ref<DirectXTextureCube> irradiance, uint32_t cubemapSize);

		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::filesystem::path& filepath) override;
		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> RT_CreateEnvironmentMap(const std::filesystem::path& filepath) override;
		virtual Ref<Texture2D> CreateBRDFLUT() override;

		virtual void GenerateMips(Ref<Image2D> image) override;
		virtual void RT_GenerateMips(Ref<Image2D> image) override;

		virtual TimeStep GetGPUTime() const override { return m_GPUTime; }

		virtual uint32_t GetCurrentFrameIndex() const override { return m_FrameIndex; }
		virtual uint32_t RT_GetCurrentFrameIndex() const override { return m_RTFrameIndex; }
		virtual Ref<RenderCommandBuffer> GetCommandBuffer() const override { return m_CommandBuffer; }
		ID3D11SamplerState* GetClampLinearSampler() const { return m_ClampLinearSampler; }

		virtual bool ResourcesCreated() const override { return m_ResourceCreated; }
		virtual const RendererCapabilities& GetCapabilities() const override { return m_Capabilities; }

		void BindFrameBuffer(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXFrameBuffer> framebuffer);
		void BindFrameBuffer(ID3D11DeviceContext* context, Ref<DirectXFrameBuffer> framebuffer);

		uint64_t GetGPUFrequncy() const { return m_GPUFrequency; }
		uint64_t HasValidFrequncy() const { return m_IsValidFrequency; }

		void RT_PrepareForSwapchainResize();

	private:
		void RT_PrepareAndBindMaterial(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXMaterial> material);
		void RT_BindResources(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<Shader> shader, std::span<const BoundResource> boundResources);
		void RT_BindPushConstants(Ref<DirectXRenderCommandBuffer> commandBuffer, Ref<DirectXPipeline> pipeline);
		void QueryCapabilities();

		void RT_BeginFrequencyQuery();
		void RT_EndFrequencyQuery();

#if 0
		void RT_FlushDXMessages();
#endif

	private:
		bool m_ResourceCreated = false;
		uint32_t m_FrameIndex = (uint32_t)-1;
		uint32_t m_RTFrameIndex = (uint32_t)-1;

		Ref<RenderCommandBuffer> m_CommandBuffer;

		VertexLayout m_QuadVertexLayout;
		Ref<DirectXVertexBuffer> m_QuadVertexBuffer;
		Ref<DirectXIndexBuffer> m_QuadIndexBuffer;

		Ref<DirectXVertexBuffer> m_CubeVertexBuffer;
		Ref<DirectXIndexBuffer> m_CubeIndexBuffer;

		ID3D11SamplerState* m_ClampLinearSampler = nullptr;

		uint32_t m_GPUTimeQuery;
		TimeStep m_GPUTime;

		ID3D11Query* m_FrequencyQuery = nullptr;
		uint64_t m_GPUFrequency = 0;
		bool m_IsValidFrequency = false;
		bool m_FrequencyQueryActive = false;
		bool m_DoFrequencyQuery = true;

		RendererCapabilities m_Capabilities;
	};


}