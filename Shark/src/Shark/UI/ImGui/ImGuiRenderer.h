#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/Image.h"

#include <imgui.h>
#include <nvrhi/nvrhi.h>

namespace Shark {

	class ImGuiTexture : public ViewableResource
	{
	public:
		virtual bool HasSampler() const override { return true; }
		virtual const ViewInfo& GetViewInfo() const override { return View; }

		ViewInfo View;
	};

	class ImGuiRenderer
	{
	public:
		bool Initialize(Ref<SwapChain> swapchain);
		void DestroyTextures();
		bool Render(ImGuiViewport* viewport);

		Ref<SwapChain> GetSwapchain() const { return m_Swapchain; }
	private:
		bool ReallocateBuffer(nvrhi::BufferHandle& buffer, uint64_t requiredSize, uint64_t reallocateSize, bool isIndexBuffer);
		void UpdateTexture(Ref<RenderCommandBuffer> commandBuffer, ImTextureData* texture);
		void DestroyTexture(ImTextureData* texture);

		nvrhi::IBindingSet* GetBindingSet(Ref<ViewableResource> viewable);
		bool UpdateGeometry(Ref<RenderCommandBuffer> commandBuffer, const ImDrawData* drawData);
	private:
		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<SwapChain> m_Swapchain;

		nvrhi::ShaderHandle m_VertexShader;
		nvrhi::ShaderHandle m_PixelShader;
		nvrhi::InputLayoutHandle m_ShaderAttribLayout;

		nvrhi::TextureHandle m_FontTexture;
		nvrhi::SamplerHandle m_FontSampler;

		nvrhi::BufferHandle m_VertexBuffer;
		nvrhi::BufferHandle m_IndexBuffer;

		nvrhi::BindingLayoutHandle m_BindingLayout;
		nvrhi::GraphicsPipelineHandle m_Pipeline;

		std::unordered_map<ViewInfo, nvrhi::BindingSetHandle> m_BindingsCache;

		std::array<std::vector<ImDrawVert>, 2> m_VertexBufferData;
		std::array<std::vector<ImDrawIdx>, 2> m_IndexBufferData;
	};

}
