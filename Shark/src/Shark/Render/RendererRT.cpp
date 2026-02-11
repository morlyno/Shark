#include "skpch.h"
#include "RendererRT.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

namespace Shark::RT {

	static void CacheSetupPipeline_LS_LSA(nvrhi::IDevice* device, ResourceCache* cache)
	{
		auto ls = Renderer::GetShaderLibrary()->Get("LinearSample");
		auto lsa = Renderer::GetShaderLibrary()->Get("LinearSampleArray");

		nvrhi::ComputePipelineDesc lsDesc;
		lsDesc.CS = ls->GetHandle(nvrhi::ShaderType::Compute);
		lsDesc.bindingLayouts = ls->GetBindingLayouts();

		nvrhi::ComputePipelineDesc lsaDesc;
		lsaDesc.CS = lsa->GetHandle(nvrhi::ShaderType::Compute);
		lsaDesc.bindingLayouts = lsa->GetBindingLayouts();

		cache->Add("Pipeline-LS", device->createComputePipeline(lsDesc));
		cache->Add("Pipeline-LSA", device->createComputePipeline(lsaDesc));
	}

	static void CacheSetupNullUAVs(nvrhi::IDevice* device, ResourceCache* cache)
	{
		nvrhi::TextureDesc desc;
		desc.format = nvrhi::Format::RGBA8_UNORM;
		desc.isUAV = true;

		cache->Add("UAV-null-0", device->createTexture(desc.setDebugName("UAV-null-0")));
		cache->Add("UAV-null-1", device->createTexture(desc.setDebugName("UAV-null-1")));
		cache->Add("UAV-null-2", device->createTexture(desc.setDebugName("UAV-null-2")));

		desc.dimension = nvrhi::TextureDimension::Texture2DArray;

		cache->Add("UAV-Array-null-0", device->createTexture(desc.setDebugName("UAV-Array-null-0")));
		cache->Add("UAV-Array-null-1", device->createTexture(desc.setDebugName("UAV-Array-null-1")));
		cache->Add("UAV-Array-null-2", device->createTexture(desc.setDebugName("UAV-Array-null-2")));
	}

	static void CacheSetupPipelineEnv(nvrhi::IDevice* device, ResourceCache* cache)
	{
		auto equirectToCubeShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		auto mipFilterShader = Renderer::GetShaderLibrary()->Get("EnvMipFilter");
		auto irradianceShader = Renderer::GetShaderLibrary()->Get("EnvIrradiance");

		nvrhi::ComputePipelineDesc etcDesc;
		etcDesc.CS = equirectToCubeShader->GetHandle(nvrhi::ShaderType::Compute);
		etcDesc.bindingLayouts = equirectToCubeShader->GetBindingLayouts();

		nvrhi::ComputePipelineDesc filterDesc;
		filterDesc.CS = mipFilterShader->GetHandle(nvrhi::ShaderType::Compute);
		filterDesc.bindingLayouts = mipFilterShader->GetBindingLayouts();

		nvrhi::ComputePipelineDesc sampleDesc;
		sampleDesc.CS = irradianceShader->GetHandle(nvrhi::ShaderType::Compute);
		sampleDesc.bindingLayouts = irradianceShader->GetBindingLayouts();

		cache->Add("Pipeline-Env-ToCube", device->createComputePipeline(etcDesc));
		cache->Add("Pipeline-Env-Filter", device->createComputePipeline(filterDesc));
		cache->Add("Pipeline-Env-Sample", device->createComputePipeline(sampleDesc));
	}

	void SetupCache(nvrhi::IDevice* device, ResourceCache* cache)
	{
		CacheSetupPipeline_LS_LSA(device, cache);
		CacheSetupNullUAVs(device, cache);
		CacheSetupPipelineEnv(device, cache);
	}

	namespace utils {

		static void BindShaderInputManager(nvrhi::GraphicsState& state, const ShaderInputManager& inputManager)
		{
			Ref<Shader> shader = inputManager.GetShader();

			for (uint32_t set = inputManager.GetStartSet(); set <= inputManager.GetEndSet(); set++)
			{
				if (shader->HasLayout(set))
					state.bindings[shader->MapSet(set)] = inputManager.GetHandle(set);
			}
		}

		static void BindShaderInputManager(nvrhi::ComputeState& state, const ShaderInputManager& inputManager)
		{
			Ref<Shader> shader = inputManager.GetShader();

			for (uint32_t set = inputManager.GetStartSet(); set <= inputManager.GetEndSet(); set++)
			{
				if (shader->HasLayout(set))
					state.bindings[shader->MapSet(set)] = inputManager.GetHandle(set);
			}
		}

		static void ClearFramebufferAttachments(nvrhi::ICommandList* commandList, Ref<FrameBuffer> framebuffer, bool isLoadClear)
		{
			auto fbHandle = framebuffer->GetHandle();
			const auto& specification = framebuffer->GetSpecification();
			const bool clearColor = !isLoadClear || specification.ClearColorOnLoad;
			const bool clearDepth = !isLoadClear || specification.ClearDepthOnLoad;

			if (clearDepth && framebuffer->HasDepthAtachment())
			{
				nvrhi::utils::ClearDepthStencilAttachment(commandList, fbHandle, specification.ClearDepthValue, specification.ClearStencilValue);
			}

			if (!clearColor)
				return;

			for (uint32_t i = 0; i < framebuffer->GetAttachmentCount(); i++)
			{
				const auto& attachment = fbHandle->getDesc().colorAttachments[i];
				const auto& clearColor = framebuffer->GetClearColor(i);

				if (nvrhi::getFormatInfo(attachment.texture->getDesc().format).kind == nvrhi::FormatKind::Integer)
					commandList->clearTextureUInt(attachment.texture, attachment.subresources, clearColor.UInt);
				else
					commandList->clearTextureFloat(attachment.texture, attachment.subresources, clearColor.Float);
			}
		}

	}

	void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, Ref<FrameBuffer> framebuffer, Ref<Shader> shader, bool expliciteClear)
	{
		SK_PROFILE_SCOPED("Renderer - BeginRenderPass");
		SK_CORE_TRACE_TAG("Renderer", "[RT] BeginRenderPass '{}'", renderPass->GetSpecification().DebugName);

		auto commandList = commandBuffer->GetHandle();
		commandList->beginMarker(renderPass->GetSpecification().DebugName.c_str());

		utils::ClearFramebufferAttachments(commandBuffer->GetHandle(), framebuffer, true);

		nvrhi::GraphicsState& graphicsState = commandBuffer->GetGraphicsState();

		graphicsState = nvrhi::GraphicsState();
		graphicsState.framebuffer = framebuffer->GetHandle();
		graphicsState.viewport.addViewport(framebuffer->GetViewport());
		graphicsState.vertexBuffers.resize(1);
		graphicsState.bindings.resize(shader->GetBindingLayouts().size());

		utils::BindShaderInputManager(graphicsState, renderPass->GetInputManager());
	}

	void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass)
	{
		SK_PROFILE_SCOPED("Renderer - EndRenderPass");
		SK_CORE_TRACE_TAG("Renderer", "[RT] EndRenderPass '{}'", renderPass->GetSpecification().DebugName);

		auto commandList = commandBuffer->GetHandle();
		commandList->endMarker();
	}

	void BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass, Ref<Shader> shader)
	{
		SK_PROFILE_SCOPED("Renderer - BeginComputePass");
		SK_CORE_TRACE_TAG("Renderer", "[RT] BeginComputePass '{}'", computePass->GetSpecification().DebugName);

		auto commandList = commandBuffer->GetHandle();
		commandList->beginMarker(computePass->GetSpecification().DebugName.c_str());

		nvrhi::ComputeState& computeState = commandBuffer->GetComputeState();
		computeState = nvrhi::ComputeState();
		computeState.bindings.resize(shader->GetBindingLayouts().size());

		utils::BindShaderInputManager(computeState, computePass->GetInputManager());
	}

	void EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass)
	{
		SK_PROFILE_SCOPED("Renderer - EndComputePass");
		SK_CORE_TRACE_TAG("Renderer", "[RT] EndComputePass '{}'", computePass->GetSpecification().DebugName);

		auto commandList = commandBuffer->GetHandle();
		commandList->endMarker();
	}

	void Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::uvec3& workGroups, const Buffer pushConstantData)
	{
		SK_PROFILE_SCOPED("Renderer - Dispatch");
		SK_CORE_TRACE_TAG("Renderer", "[RT] Dispatch '{}' '{}' {}", pipeline->GetDebugName(), material ? material->GetName() : "<null>", workGroups);

		nvrhi::ComputeState& computeState = commandBuffer->GetComputeState();
		computeState.pipeline = pipeline->GetHandle();

		if (material)
		{
			utils::BindShaderInputManager(computeState, material->GetInputManager());
		}

		auto commandList = commandBuffer->GetHandle();
		commandList->setComputeState(computeState);
		commandList->setPushConstants(pushConstantData.As<const void>(), pushConstantData.Size);

		commandList->dispatch(workGroups.x, workGroups.y, workGroups.z);
	}

	void RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const nvrhi::DrawArguments& drawArguments, Buffer pushConstant)
	{
		SK_PROFILE_SCOPED("Renderer - RenderGeometry");
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderGeometry '{}' '{}'", material->GetName(), pipeline->GetSpecification().DebugName);

		nvrhi::GraphicsState graphicsState = commandBuffer->GetGraphicsState();

		graphicsState.pipeline = pipeline->GetHandle();

		nvrhi::VertexBufferBinding vbufBinding;
		vbufBinding.buffer = vertexBuffer->GetHandle();
		vbufBinding.slot = 0;
		vbufBinding.offset = 0;
		graphicsState.vertexBuffers[0] = vbufBinding;

		if (material)
		{
			utils::BindShaderInputManager(graphicsState, material->GetInputManager());
		}

		graphicsState.indexBuffer.buffer = indexBuffer->GetHandle();
		graphicsState.indexBuffer.format = nvrhi::Format::R32_UINT;
		graphicsState.indexBuffer.offset = 0;

		auto commandList = commandBuffer->GetHandle();
		commandList->setGraphicsState(graphicsState);
		commandList->setPushConstants(pushConstant.As<const void>(), pushConstant.Size);
		commandList->drawIndexed(drawArguments);
	}

	void RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, const nvrhi::DrawArguments& drawArguments, const Buffer pushConstant)
	{
		SK_PROFILE_SCOPED("Renderer - RenderGeometry");
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderGeometry '{}' '{}'", material->GetName(), pipeline->GetSpecification().DebugName);

		nvrhi::GraphicsState graphicsState = commandBuffer->GetGraphicsState();

		graphicsState.pipeline = pipeline->GetHandle();

		nvrhi::VertexBufferBinding vbufBinding;
		vbufBinding.buffer = vertexBuffer->GetHandle();
		vbufBinding.slot = 0;
		vbufBinding.offset = 0;
		graphicsState.vertexBuffers[0] = vbufBinding;

		if (material)
		{
			utils::BindShaderInputManager(graphicsState, material->GetInputManager());
		}

		auto commandList = commandBuffer->GetHandle();
		commandList->setGraphicsState(graphicsState);
		commandList->setPushConstants(pushConstant.As<const void>(), pushConstant.Size);
		commandList->drawIndexed(drawArguments);
	}

	void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, const Buffer pushConstantsData)
	{
		SK_PROFILE_SCOPED("Renderer - RenderSubmesh");
		SK_CORE_TRACE_TAG("Renderer", "[RT] RenderSubmesh '{}':{} '{}'", meshSource->GetName(), submeshIndex, material->GetName());

		auto vertexBuffer = meshSource->GetVertexBuffer();
		auto indexBuffer = meshSource->GetIndexBuffer();

		nvrhi::GraphicsState& drawState = commandBuffer->GetGraphicsState();

		drawState.pipeline = pipeline->GetHandle();

		nvrhi::VertexBufferBinding vbufBinding;
		vbufBinding.buffer = vertexBuffer->GetHandle();
		vbufBinding.slot = 0;
		vbufBinding.offset = 0;
		drawState.vertexBuffers[0] = vbufBinding;

		if (material)
		{
			utils::BindShaderInputManager(drawState, material->GetInputManager());
		}

		drawState.indexBuffer.buffer = indexBuffer->GetHandle();
		drawState.indexBuffer.format = nvrhi::Format::R32_UINT;
		drawState.indexBuffer.offset = 0;

		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];

		nvrhi::DrawArguments drawArgs;
		drawArgs.vertexCount = submesh.IndexCount;
		drawArgs.startIndexLocation = submesh.BaseIndex;
		drawArgs.startVertexLocation = submesh.BaseVertex;

		auto commandList = commandBuffer->GetHandle();
		commandList->setGraphicsState(drawState);
		commandList->setPushConstants(pushConstantsData.As<const void>(), pushConstantsData.Size);
		commandList->drawIndexed(drawArgs);
	}


	void WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData)
	{
		auto commandList = commandBuffer->GetHandle();
		commandList->writeBuffer(buffer->GetHandle(), bufferData.As<const void>(), bufferData.Size);

	}

	void WriteImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image, const ImageSlice& slice, const Buffer imageData)
	{
		auto commandList = commandBuffer->GetHandle();
		auto imageHandle = image->GetHandle();
		commandList->writeTexture(imageHandle, slice.Layer, slice.Mip, imageData.As<const void>(), imageHandle->getDesc().width * nvrhi::getFormatInfo(imageHandle->getDesc().format).bytesPerBlock);
	}

	namespace Internal {

		template<typename T0, typename T1>
		void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<T0> sourceImage, const ImageSlice& sourceSlice, Ref<T1> destinationImage, const ImageSlice& destinationSlice)
		{
			SK_PROFILE_SCOPED("Renderer - CopySlice");
			SK_CORE_TRACE_TAG("Renderer", "[RT] CopySlice '{}':(Mip:{}, Level: {}) -> '{}':(Mip:{}, Level: {})", sourceImage->GetSpecification().DebugName, sourceSlice.Mip, sourceSlice.Layer, destinationImage->GetSpecification().DebugName, destinationSlice.Mip, destinationSlice.Layer);

			auto commandList = commandBuffer->GetHandle();
			auto srcSlice = nvrhi::TextureSlice().setMipLevel(sourceSlice.Mip).setArraySlice(sourceSlice.Layer);
			auto dstSlice = nvrhi::TextureSlice().setMipLevel(destinationSlice.Mip).setArraySlice(destinationSlice.Layer);

			auto srcTex = sourceImage->GetHandle();
			auto dstTex = destinationImage->GetHandle();

			commandList->copyTexture(dstTex, dstSlice, srcTex, srcSlice);
		}

		template<typename T0, typename T1>
		void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<T0> sourceImage, uint32_t sourceMip, Ref<T1> destinationImage, uint32_t destinationMip)
		{
			SK_PROFILE_SCOPED("Renderer - CopyMip");
			SK_CORE_TRACE_TAG("Renderer", "[RT] CopyMip '{}':{} -> '{}':{}", sourceImage->GetSpecification().DebugName, sourceMip, destinationImage->GetSpecification().DebugName, destinationMip);

			auto commandList = commandBuffer->GetHandle();
			auto srcSlice = nvrhi::TextureSlice().setMipLevel(sourceMip);
			auto dstSlice = nvrhi::TextureSlice().setMipLevel(destinationMip);

			auto srcTex = sourceImage->GetHandle();
			auto dstTex = destinationImage->GetHandle();

			for (uint32_t layer = 0; layer < srcTex->getDesc().arraySize; layer++)
			{
				srcSlice.arraySlice = layer;
				dstSlice.arraySlice = layer;

				commandList->copyTexture(dstTex, dstSlice,
										 srcTex, srcSlice);
			}
		}

		template<typename T0, typename T1>
		void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<T0> sourceImage, Ref<T1> destinationImage)
		{
			SK_PROFILE_SCOPED("Renderer - CopyImage");
			SK_CORE_TRACE_TAG("Renderer", "[RT] CopyImage '{}' -> '{}'", sourceImage->GetSpecification().DebugName, destinationImage->GetSpecification().DebugName);

			auto commandList = commandBuffer->GetHandle();
			auto slice = nvrhi::TextureSlice();

			auto srcTex = sourceImage->GetHandle();
			auto dstTex = destinationImage->GetHandle();

			for (uint32_t mip = 0; mip < srcTex->getDesc().mipLevels; mip++)
			{
				slice.mipLevel = mip;

				for (uint32_t layer = 0; layer < srcTex->getDesc().arraySize; layer++)
				{
					slice.arraySlice = layer;
					commandList->copyTexture(dstTex, slice, srcTex, slice);
				}
			}
		}

	}

	void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		Internal::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D> destinationImage, const ImageSlice& destinationSlice)
	{
		Internal::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, const ImageSlice& sourceSlice, Ref<StagingImage2D> destinationImage, const ImageSlice& destinationSlice)
	{
		Internal::CopySlice(commandBuffer, sourceImage, sourceSlice, destinationImage, destinationSlice);
	}

	void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		Internal::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, uint32_t sourceMip, Ref<Image2D> destinationImage, uint32_t destinationMip)
	{
		Internal::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, uint32_t sourceMip, Ref<StagingImage2D> destinationImage, uint32_t destinationMip)
	{
		Internal::CopyMip(commandBuffer, sourceImage, sourceMip, destinationImage, destinationMip);
	}

	void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		Internal::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, Ref<Image2D> destinationImage)
	{
		Internal::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> sourceImage, Ref<StagingImage2D> destinationImage)
	{
		Internal::CopyImage(commandBuffer, sourceImage, destinationImage);
	}

	void CopyTexture(nvrhi::ICommandList* commandList, nvrhi::ITexture* source, nvrhi::ITexture* destination)
	{
		auto slice = nvrhi::TextureSlice();

		for (uint32_t mip = 0; mip < source->getDesc().mipLevels; mip++)
		{
			slice.mipLevel = mip;

			for (uint32_t layer = 0; layer < source->getDesc().arraySize; layer++)
			{
				slice.arraySlice = layer;
				commandList->copyTexture(destination, slice, source, slice);
			}
		}
	}

}
