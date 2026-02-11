#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/RenderPass.h"
#include "Shark/Render/ComputePass.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/ComputePipeline.h"
#include "Shark/Render/Material.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	class ResourceCache;

	namespace RT {

		void SetupCache(nvrhi::IDevice* device, ResourceCache* cache);

		void BeginRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass, Ref<FrameBuffer> framebuffer, Ref<Shader> shader, bool expliciteClear);
		void EndRenderPass(Ref<RenderCommandBuffer> commandBuffer, Ref<RenderPass> renderPass);
		void BeginComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass, Ref<Shader> shader);
		void EndComputePass(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePass> computePass);
		void Dispatch(Ref<RenderCommandBuffer> commandBuffer, Ref<ComputePipeline> pipeline, Ref<Material> material, const glm::uvec3& workGroups, const Buffer pushConstantData);
		void RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const nvrhi::DrawArguments& drawArguments, Buffer pushConstant);
		void RenderGeometry(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, const nvrhi::DrawArguments& drawArguments, const Buffer pushConstant);
		void RenderSubmesh(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, const Buffer pushConstantsData);

		void WriteBuffer(Ref<RenderCommandBuffer> commandBuffer, Ref<GpuBuffer> buffer, const Buffer bufferData);
		void WriteImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image, const ImageSlice& slice, const Buffer imageData);

		void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, const ImageSlice& sourceSlice, Ref<Image2D>        destinationImage, const ImageSlice& destinationSlice);
		void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, const ImageSlice& sourceSlice, Ref<Image2D>        destinationImage, const ImageSlice& destinationSlice);
		void CopySlice(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, const ImageSlice& sourceSlice, Ref<StagingImage2D> destinationImage, const ImageSlice& destinationSlice);
		void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, uint32_t sourceMip, Ref<Image2D>        destinationImage, uint32_t destinationMip);
		void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, uint32_t sourceMip, Ref<Image2D>        destinationImage, uint32_t destinationMip);
		void CopyMip(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, uint32_t sourceMip, Ref<StagingImage2D> destinationImage, uint32_t destinationMip);
		void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, Ref<Image2D>        destinationImage);
		void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<StagingImage2D> sourceImage, Ref<Image2D>        destinationImage);
		void CopyImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D>        sourceImage, Ref<StagingImage2D> destinationImage);
		void CopyTexture(nvrhi::ICommandList* commandList, nvrhi::ITexture* source, nvrhi::ITexture* destination);

	}

}
