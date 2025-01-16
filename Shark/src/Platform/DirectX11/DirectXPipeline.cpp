#include "skpch.h"
#include "DirectXPipeline.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"

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

		static D3D11_COMPARISON_FUNC ToD3D11Comparison(CompareOperator compareOperator)
		{
			switch (compareOperator)
			{
				case CompareOperator::Equal: return D3D11_COMPARISON_EQUAL;
				case CompareOperator::NotEqual: return D3D11_COMPARISON_NOT_EQUAL;
				case CompareOperator::Less: return D3D11_COMPARISON_LESS;
				case CompareOperator::Greater: return D3D11_COMPARISON_GREATER;
				case CompareOperator::LessEqual: return D3D11_COMPARISON_LESS_EQUAL;
				case CompareOperator::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
				case CompareOperator::Never: return D3D11_COMPARISON_NEVER;
				case CompareOperator::Always: return D3D11_COMPARISON_ALWAYS;
			}

			SK_CORE_ASSERT(false, "Unkown DepthCompareOperator");
			return (D3D11_COMPARISON_FUNC)0;
		}

		static D3D11_STENCIL_OP ToD3D11StencilOp(StencilOperation operation)
		{
			switch (operation)
			{
				case StencilOperation::Keep: return D3D11_STENCIL_OP_KEEP;
				case StencilOperation::Zero: return D3D11_STENCIL_OP_ZERO;
				case StencilOperation::Replace: return D3D11_STENCIL_OP_REPLACE;
				case StencilOperation::IncrementClamp: return D3D11_STENCIL_OP_INCR_SAT;
				case StencilOperation::DecrementClamp: return D3D11_STENCIL_OP_DECR_SAT;
				case StencilOperation::Invert: return D3D11_STENCIL_OP_INVERT;
				case StencilOperation::IncrementWrap: return D3D11_STENCIL_OP_INCR;
				case StencilOperation::DecrementWrap: return D3D11_STENCIL_OP_DECR;
			}

			SK_CORE_ASSERT(false, "Unkown StencilOperation");
			return (D3D11_STENCIL_OP)0;
		}

	}

	DirectXPipeline::DirectXPipeline(const PipelineSpecification& specs)
		: m_Specification(specs)
	{
		RT_Init();
	}

	DirectXPipeline::~DirectXPipeline()
	{
		Renderer::SubmitResourceFree([rasterizer = m_RasterizerState, depthStencil = m_DepthStencilState, inputLayout = m_InputLayout, pcBuffer = m_PushConstant.Buffer]()
		{
			if (rasterizer)
				rasterizer->Release();
			if (depthStencil)
				depthStencil->Release();
			if (inputLayout)
				inputLayout->Release();
			if (pcBuffer)
				pcBuffer->Release();
		});
	}

	void DirectXPipeline::SetPushConstant(Ref<RenderCommandBuffer> commandBuffer, Buffer pushConstantData)
	{
		Ref instance = this;
		Renderer::Submit([instance, commandBuffer, tempBuffer = Buffer::Copy(pushConstantData)]() mutable
		{
			instance->RT_SetPushConstant(commandBuffer, tempBuffer);
			tempBuffer.Release();
		});
	}

	void DirectXPipeline::RT_SetPushConstant(Ref<RenderCommandBuffer> commandBuffer, Buffer pushConstantData)
	{
		auto cmd = commandBuffer.As<DirectXRenderCommandBuffer>()->GetContext();
		cmd->UpdateSubresource(m_PushConstant.Buffer, 0, nullptr, pushConstantData.Data, m_PushConstant.Size, 0);
	}

	void DirectXPipeline::SetFrameBuffer(Ref<FrameBuffer> frameBuffer)
	{
		m_FrameBuffer = frameBuffer.As<DirectXFrameBuffer>();
		m_Specification.TargetFrameBuffer = frameBuffer;
	}

	void DirectXPipeline::RT_Init()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		m_Shader = m_Specification.Shader.As<DirectXShader>();
		m_FrameBuffer = m_Specification.TargetFrameBuffer.As<DirectXFrameBuffer>();

		// Rasterizer
		{
			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.FillMode = m_Specification.WireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
			desc.CullMode = m_Specification.BackFaceCulling ? D3D11_CULL_BACK : D3D11_CULL_NONE;

			DX11_VERIFY(dxDevice->CreateRasterizerState(&desc, &m_RasterizerState));
		}

		// DepthStencil
		{
			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthEnable = m_Specification.DepthEnabled;
			desc.DepthWriteMask = m_Specification.WriteDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = utils::ToD3D11Comparison(m_Specification.DepthOperator);

			desc.StencilEnable = m_Specification.EnableStencil;
			desc.StencilReadMask = m_Specification.StencilReadMask;
			desc.StencilWriteMask = m_Specification.StencilWriteMask;
			
			desc.FrontFace.StencilFailOp = utils::ToD3D11StencilOp(m_Specification.StencilFailOperation);
			desc.FrontFace.StencilDepthFailOp = utils::ToD3D11StencilOp(m_Specification.StencilDepthFailOperation);
			desc.FrontFace.StencilPassOp = utils::ToD3D11StencilOp(m_Specification.StencilPassOperation);
			desc.FrontFace.StencilFunc = utils::ToD3D11Comparison(m_Specification.StencilComparisonOperator);
			desc.BackFace = desc.FrontFace;

			DX11_VERIFY(dxDevice->CreateDepthStencilState(&desc, &m_DepthStencilState));
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
		DX11_VERIFY(dxDevice->CreateInputLayout(inputElements.data(), (UINT)inputElements.size(), shaderBinaryWithInputSignature.data(), shaderBinaryWithInputSignature.size(), &m_InputLayout));

		const auto& reflectionData = m_Specification.Shader->GetReflectionData();
		if (reflectionData.HasPushConstant)
		{
			uint32_t size = reflectionData.PushConstant.StructSize;
			m_PushConstant.RequestedSize = size;
			m_PushConstant.Size = size + (16 - size % 16) % 16;
			m_PushConstant.Binding = reflectionData.PushConstant.DXBinding;

			D3D11_BUFFER_DESC bufferDesc = {};
			bufferDesc.ByteWidth = m_PushConstant.Size;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			DirectXAPI::CreateBuffer(dxDevice, bufferDesc, nullptr, m_PushConstant.Buffer);
			DirectXAPI::SetDebugName(m_PushConstant.Buffer, fmt::format("{}-PushConstant", m_Specification.DebugName));
		}

	}

}
