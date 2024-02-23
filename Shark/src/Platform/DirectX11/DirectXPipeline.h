#pragma once

#include "Shark/Render/Pipeline.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXPipeline : public Pipeline
	{
	public:
		DirectXPipeline(const PipelineSpecification& specs);
		virtual ~DirectXPipeline();

		virtual void SetPushConstant(Buffer pushConstantData) override;
		void RT_SetPushConstant(Buffer pushConstantData);

		virtual void SetFrameBuffer(Ref<FrameBuffer> frameBuffer) override;
		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }

		void BeginRenderPass();
		void EndRenderPass();

		bool UsesPushConstant() const { return m_PushConstant.Size != 0; }
		Ref<ConstantBuffer> GetLastUpdatedPushConstantBuffer();

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
			uint32_t BufferIndex = 0;
			std::vector<Ref<ConstantBuffer>> Buffers;

			uint32_t Size = 0;
			uint32_t Binding = 0;
		};
		PushConstant m_PushConstant;

		friend class DirectXRenderer;
	};

}
