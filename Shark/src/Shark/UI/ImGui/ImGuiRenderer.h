#pragma once

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/Image.h"

#include <imgui.h>
#include <nvrhi/nvrhi.h>

namespace Shark {

	class ImGuiRenderer
	{
	public:
		bool Initialize();
		bool Render(ImGuiViewport* viewport, nvrhi::GraphicsPipelineHandle pipeline, nvrhi::FramebufferHandle framebuffer);
		bool RenderToSwapchain(ImGuiViewport* viewport, Ref<SwapChain> swapchain);

		void OnDestroySwapchain(Ref<SwapChain> swapchain);

	private:
		bool ReallocateBuffer(nvrhi::BufferHandle& buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer);
		void UpdateTexture(nvrhi::CommandListHandle commandList, ImTextureData* texture);


		nvrhi::IBindingSet* GetBindingSet(const ViewInfo* viewInfo);
		bool UpdateGeometry(nvrhi::ICommandList* commandList);
	private:
		Ref<RenderCommandBuffer> m_CommandBuffer;

		nvrhi::ShaderHandle m_VertexShader;
		nvrhi::ShaderHandle m_PixelShader;
		nvrhi::InputLayoutHandle m_ShaderAttribLayout;

		nvrhi::TextureHandle m_FontTexture;
		nvrhi::SamplerHandle m_FontSampler;

		nvrhi::BufferHandle m_VertexBuffer;
		nvrhi::BufferHandle m_IndexBuffer;

		nvrhi::BindingLayoutHandle m_BindingLayout;
		nvrhi::GraphicsPipelineDesc m_BasePSODesc;

		std::unordered_map<const ViewInfo*, nvrhi::BindingSetHandle> m_BindingsCache;

		std::vector<ImDrawVert> m_VertexBufferData;
		std::vector<ImDrawIdx> m_IndexBufferData;

		std::unordered_map<SwapChain*, nvrhi::GraphicsPipelineHandle> m_PipelineCache;
	};

}
