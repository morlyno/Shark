#include "skpch.h"
#include "DirectXPipeline.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	namespace utils {

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
		RT_Init();
	}

	DirectXPipeline::~DirectXPipeline()
	{
		Renderer::SubmitResourceFree([rasterizer = m_RasterizerState, depthStencil = m_DepthStencilState, inputLayout = m_InputLayout]()
		{
			if (rasterizer)
				rasterizer->Release();
			if (depthStencil)
				depthStencil->Release();
			if (inputLayout)
				inputLayout->Release();
		});
	}

	void DirectXPipeline::SetPushConstant(Buffer pushConstantData)
	{
		Buffer data = Buffer::Copy(pushConstantData);

		Ref<DirectXPipeline> instance = this;
		Renderer::Submit([instance, data]() mutable
		{
			instance->RT_SetPushConstant(data);
			data.Release();
		});
	}

	void DirectXPipeline::RT_SetPushConstant(Buffer pushConstantData)
	{
		SK_CORE_VERIFY(pushConstantData.Size <= m_PushConstant.Size);

		if (m_PushConstant.Buffers.size() == m_PushConstant.BufferIndex)
			m_PushConstant.Buffers.push_back(ConstantBuffer::Create(m_PushConstant.Size));

		Ref<ConstantBuffer> constantBuffer = m_PushConstant.Buffers[m_PushConstant.BufferIndex++];
		constantBuffer->RT_UploadData(pushConstantData);
	}

	void DirectXPipeline::SetFrameBuffer(Ref<FrameBuffer> frameBuffer)
	{
		m_FrameBuffer = frameBuffer.As<DirectXFrameBuffer>();
		m_Specification.TargetFrameBuffer = frameBuffer;
	}

	void DirectXPipeline::BeginRenderPass()
	{
		m_PushConstant.BufferIndex = 0;
	}

	void DirectXPipeline::EndRenderPass()
	{

	}

	Ref<ConstantBuffer> DirectXPipeline::GetLastUpdatedPushConstantBuffer()
	{
		const uint32_t index = m_PushConstant.BufferIndex - 1;
		SK_CORE_VERIFY(m_PushConstant.BufferIndex != 0 && index < m_PushConstant.Buffers.size());
		return m_PushConstant.Buffers[index];
	}

	void DirectXPipeline::RT_Init()
	{
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
			desc.DepthFunc = utils::ToD3D11Comparison(m_Specification.DepthOperator);

			SK_DX11_CALL(dev->CreateDepthStencilState(&desc, &m_DepthStencilState));
		}

		m_PrimitveTopology = utils::SharkPrimitveTopologyToD3D11(m_Specification.Primitve);

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;
		for (auto& element : m_Specification.Layout)
		{
			D3D11_INPUT_ELEMENT_DESC inputElementDesc{};
			inputElementDesc.SemanticName = element.Semantic.c_str();
			inputElementDesc.SemanticIndex = 0;
			inputElementDesc.Format = utils::VertexDataTypeToDXGI_FORMAT(element.Type);
			inputElementDesc.InputSlot = 0;
			inputElementDesc.AlignedByteOffset = element.Offset;
			inputElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			inputElementDesc.InstanceDataStepRate = 0;
			inputElements.emplace_back(inputElementDesc);
		}
		const auto& shaderBinaryWithInputSignature = m_Specification.Shader.As<DirectXShader>()->GetShaderBinaries().at(ShaderUtils::ShaderStage::Vertex);
		SK_DX11_CALL(dev->CreateInputLayout(inputElements.data(), (UINT)inputElements.size(), shaderBinaryWithInputSignature.data(), shaderBinaryWithInputSignature.size(), &m_InputLayout));

		const auto& reflectionData = m_Specification.Shader->GetReflectionData();
		if (reflectionData.HasPushConstant)
		{
			m_PushConstant.Size = reflectionData.PushConstant.StructSize;
			m_PushConstant.Binding = reflectionData.PushConstant.DXBinding;
		}

	}

}
