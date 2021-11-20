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

		virtual void SetFrameBuffer(Ref<FrameBuffer> frameBuffer) override;

		virtual const PipelineSpecification& GetSpecification() const override { return m_Specification; }

	private:
		PipelineSpecification m_Specification;
		Ref<DirectXShader> m_Shader;
		Ref<DirectXFrameBuffer> m_FrameBuffer;

		ID3D11RasterizerState* m_RasterizerState = nullptr;
		ID3D11DepthStencilState* m_DepthStencilState = nullptr;

		D3D_PRIMITIVE_TOPOLOGY m_PrimitveTopology;

		friend class DirectXRenderer;
	};

}
