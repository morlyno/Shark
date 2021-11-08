#include "skpch.h"
#include "DirectXPipeline.h"

#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	namespace Utils {

		DXGI_FORMAT VertexDataTypeToDXGI_FORMAT(VertexDataType type)
		{
			switch (type)
			{
				case VertexDataType::None:    return DXGI_FORMAT_UNKNOWN;
				case VertexDataType::Float:   return DXGI_FORMAT_R32_FLOAT;
				case VertexDataType::Float2:  return DXGI_FORMAT_R32G32_FLOAT;
				case VertexDataType::Float3:  return DXGI_FORMAT_R32G32B32_FLOAT;
				case VertexDataType::Float4:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case VertexDataType::Int:     return DXGI_FORMAT_R32G32B32A32_SINT;
				case VertexDataType::Int2:    return DXGI_FORMAT_R32_SINT;
				case VertexDataType::Int3:	  return DXGI_FORMAT_R32G32_SINT;
				case VertexDataType::Int4:	  return DXGI_FORMAT_R32G32B32_SINT;
				case VertexDataType::Bool:    return DXGI_FORMAT_R32_UINT;
			}
			SK_CORE_ASSERT(false, "Unkown VertexDataType");
			return (DXGI_FORMAT)0;
		}

	}

	DirectXPipeline::DirectXPipeline(const PipelineSpecification& specs)
		: m_Specification(specs)
	{
		ID3D11Device* dev = DirectXRenderer::GetDevice();

		m_Shader = m_Specification.Shader.As<DirectXShader>();
		m_FrameBuffer = m_Specification.TargetFrameBuffer.As<DirectXFrameBuffer>();

		// Rasterizer
		{
			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.FillMode = specs.WireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
			desc.CullMode = specs.BackFaceCulling ? D3D11_CULL_BACK : D3D11_CULL_NONE;

			HRESULT hr = dev->CreateRasterizerState(&desc, &m_RasterizerState);
			SK_CORE_ASSERT(SUCCEEDED(hr), fmt::format("D3D11Device::CreateRasterizerState Failed! {}:{}", __FILE__, __LINE__))
		}

		// DepthStencil
		{
			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthEnable = specs.DepthEnabled;
			desc.DepthWriteMask = specs.WriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			
			HRESULT hr = dev->CreateDepthStencilState(&desc, &m_DepthStencilState);
			SK_CORE_ASSERT(SUCCEEDED(hr), fmt::format("D3D11Device::CreateDepthStencilState Failed! {}:{}", __FILE__, __LINE__))
		}

	}

	DirectXPipeline::~DirectXPipeline()
	{
		if (m_RasterizerState)
			m_RasterizerState->Release();
		if (m_DepthStencilState)
			m_DepthStencilState->Release();
	}

	void DirectXPipeline::SetFrameBuffer(Ref<FrameBuffer> frameBuffer)
	{
		m_FrameBuffer = frameBuffer.As<DirectXFrameBuffer>();
		m_Specification.TargetFrameBuffer = frameBuffer;
	}

}
