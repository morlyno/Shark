#include "skpch.h"
#include "DirectXPipeline.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	namespace Utils {

		static DXGI_FORMAT VertexDataTypeToDXGI_FORMAT(VertexDataType type)
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

		static D3D11_PRIMITIVE_TOPOLOGY SharkPrimitveTopologyToD3D11(PrimitveType topology)
		{
			switch (topology)
			{
			case PrimitveType::Triangle:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitveType::Line:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitveType::Dot:       return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			}

			SK_CORE_ASSERT(false, "Unkonw Topology");
			return (D3D11_PRIMITIVE_TOPOLOGY)0;
		}

		static D3D11_COMPARISON_FUNC ToD3D11Comparison(DepthCompareOperator compareOperator)
		{
			switch (compareOperator)
			{
				case DepthCompareOperator::Equal: return D3D11_COMPARISON_EQUAL;
				case DepthCompareOperator::Less: return D3D11_COMPARISON_LESS;
				case DepthCompareOperator::Greater: return D3D11_COMPARISON_GREATER;
				case DepthCompareOperator::LessEqual: return D3D11_COMPARISON_LESS_EQUAL;
				case DepthCompareOperator::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
			}

			SK_CORE_ASSERT(false, "Unkown DepthCompareOperator");
			return (D3D11_COMPARISON_FUNC)0;
		}

	}

	DirectXPipeline::DirectXPipeline(const PipelineSpecification& specs)
		: m_Specification(specs)
	{
		Ref<DirectXPipeline> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Init();
		});
	}

	DirectXPipeline::~DirectXPipeline()
	{
		Renderer::SubmitResourceFree([rasterizer = m_RasterizerState, depthStencil = m_DepthStencilState]()
		{
			if (rasterizer)
				rasterizer->Release();
			if (depthStencil)
				depthStencil->Release();
		});
	}

	void DirectXPipeline::SetFrameBuffer(Ref<FrameBuffer> frameBuffer)
	{
		m_FrameBuffer = frameBuffer.As<DirectXFrameBuffer>();
		m_Specification.TargetFrameBuffer = frameBuffer;
	}

	void DirectXPipeline::RT_Init()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		ID3D11Device* dev = DirectXRenderer::GetDevice();

		m_Shader = m_Specification.Shader.As<DirectXShader>();
		m_FrameBuffer = m_Specification.TargetFrameBuffer.As<DirectXFrameBuffer>();

		// Rasterizer
		{
			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.FillMode = m_Specification.WireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
			desc.CullMode = m_Specification.BackFaceCulling ? D3D11_CULL_BACK : D3D11_CULL_NONE;

			SK_DX11_CALL(dev->CreateRasterizerState(&desc, &m_RasterizerState));
		}

		// DepthStencil
		{
			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthEnable = m_Specification.DepthEnabled;
			desc.DepthWriteMask = m_Specification.WriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = Utils::ToD3D11Comparison(m_Specification.DepthOperator);

			SK_DX11_CALL(dev->CreateDepthStencilState(&desc, &m_DepthStencilState));
		}

		m_PrimitveTopology = Utils::SharkPrimitveTopologyToD3D11(m_Specification.Primitve);
	}

}
