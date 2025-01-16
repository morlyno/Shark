#pragma once

#include "Shark/Render/Pipeline.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXPipeline : public Pipeline
	{
	public:
		DirectXPipeline(const PipelineSpecification& specs);
		virtual ~DirectXPipeline();

		virtual void SetPushConstant(Ref<RenderCommandBuffer> commandBuffer, Buffer pushConstantData) override;
		virtual void RT_SetPushConstant(Ref<RenderCommandBuffer> commandBuffer, Buffer pushConstantData) override;

		virtual void SetFrameBuffer(Ref<FrameBuffer> frameBuffer) override;
		virtual PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }

		bool UsesPushConstant() const { return m_PushConstant.Size; }

	private:
		void RT_Init();

	private:
		PipelineSpecification m_Specification;
		Ref<DirectXShader> m_Shader;
		Ref<DirectXFrameBuffer> m_FrameBuffer;

		ID3D11RasterizerState* m_RasterizerState = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;
		ID3D11InputLayout* m_InputLayout = nullptr;

		D3D_PRIMITIVE_TOPOLOGY m_PrimitveTopology;

		struct PushConstant
		{
			ID3D11Buffer* Buffer = nullptr;
			uint32_t RequestedSize = 0;
			uint32_t Size = 0; // Aligned to a multiple of 16
			uint32_t Binding = 0;
		};
		PushConstant m_PushConstant;

		friend class DirectXRenderer;
	};

}
