#include "skpch.h"
#include "Pipeline.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	namespace utils {

		static nvrhi::PrimitiveType ConvertPrimitiveType(PrimitiveType primitive)
		{
			switch (primitive)
			{
				case PrimitiveType::Triangle: return nvrhi::PrimitiveType::TriangleList;
				case PrimitiveType::Line: return nvrhi::PrimitiveType::LineList;
				case PrimitiveType::Dot: return nvrhi::PrimitiveType::PointList;
			}
			SK_CORE_ASSERT(false, "Unknown PrimitiveType");
			return nvrhi::PrimitiveType::TriangleList;
		}

		static nvrhi::ComparisonFunc ConvertOperator(CompareOperator depthOperator)
		{
			switch (depthOperator)
			{
				case Shark::CompareOperator::Equal: return nvrhi::ComparisonFunc::Equal;
				case Shark::CompareOperator::NotEqual: return nvrhi::ComparisonFunc::NotEqual;
				case Shark::CompareOperator::Less: return nvrhi::ComparisonFunc::Less;
				case Shark::CompareOperator::Greater: return nvrhi::ComparisonFunc::Greater;
				case Shark::CompareOperator::LessEqual: return nvrhi::ComparisonFunc::LessOrEqual;
				case Shark::CompareOperator::GreaterEqual: return nvrhi::ComparisonFunc::GreaterOrEqual;
				case Shark::CompareOperator::Never: return nvrhi::ComparisonFunc::Never;
				case Shark::CompareOperator::Always: return nvrhi::ComparisonFunc::Always;
			}
			SK_CORE_ASSERT(false, "Unknown CompareOperator");
			return nvrhi::ComparisonFunc::Less;
		}

		static nvrhi::StencilOp ConvertStencilOperation(StencilOperation operation)
		{
			switch (operation)
			{
				case StencilOperation::Keep: return nvrhi::StencilOp::Keep;
				case StencilOperation::Zero: return nvrhi::StencilOp::Zero;
				case StencilOperation::Replace: return nvrhi::StencilOp::Replace;
				case StencilOperation::IncrementClamp: return nvrhi::StencilOp::IncrementAndClamp;
				case StencilOperation::DecrementClamp: return nvrhi::StencilOp::DecrementAndClamp;
				case StencilOperation::Invert: return nvrhi::StencilOp::Invert;
				case StencilOperation::IncrementWrap: return nvrhi::StencilOp::IncrementAndWrap;
				case StencilOperation::DecrementWrap: return nvrhi::StencilOp::DecrementAndWrap;
			}
			SK_CORE_ASSERT(false, "Unknown StencilOperation");
			return nvrhi::StencilOp::Keep;
		}

		static nvrhi::Format ConvertVertexDataType(VertexDataType type)
		{
			switch (type)
			{
				case VertexDataType::None: return nvrhi::Format::UNKNOWN;
				case VertexDataType::Float: return nvrhi::Format::R32_FLOAT;
				case VertexDataType::Float2: return nvrhi::Format::RG32_FLOAT;
				case VertexDataType::Float3: return nvrhi::Format::RGB32_FLOAT;
				case VertexDataType::Float4: return nvrhi::Format::RGBA32_FLOAT;
				case VertexDataType::Int: return nvrhi::Format::R32_SINT;
				case VertexDataType::Int2: return nvrhi::Format::RG32_SINT;
				case VertexDataType::Int3: return nvrhi::Format::RGB32_SINT;
				case VertexDataType::Int4: return nvrhi::Format::RGBA32_SINT;
				case VertexDataType::Bool: return nvrhi::Format::R32_UINT;
			}
			SK_CORE_ASSERT(false, "Unknown VertexDataType");
			return nvrhi::Format::UNKNOWN;
		}

	}

	Pipeline::Pipeline(const PipelineSpecification& specification, const nvrhi::FramebufferInfo& framebufferInfo)
		: m_Specification(specification)
	{
		auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
			.setPrimType(utils::ConvertPrimitiveType(m_Specification.Primitve))
			.setVertexShader(m_Specification.Shader->GetHandle(nvrhi::ShaderType::Vertex))
			.setHullShader(m_Specification.Shader->GetHandle(nvrhi::ShaderType::Hull))
			.setDomainShader(m_Specification.Shader->GetHandle(nvrhi::ShaderType::Domain))
			.setGeometryShader(m_Specification.Shader->GetHandle(nvrhi::ShaderType::Geometry))
			.setPixelShader(m_Specification.Shader->GetHandle(nvrhi::ShaderType::Pixel));
		
		auto stencilOpDesc = nvrhi::DepthStencilState::StencilOpDesc()
			.setFailOp(utils::ConvertStencilOperation(m_Specification.StencilFailOperation))
			.setDepthFailOp(utils::ConvertStencilOperation(m_Specification.StencilDepthFailOperation))
			.setPassOp(utils::ConvertStencilOperation(m_Specification.StencilPassOperation))
			.setStencilFunc(utils::ConvertOperator(m_Specification.StencilComparisonOperator));

		for (uint32_t i = 0; i < framebufferInfo.colorFormats.size(); i++)
		{
			auto& blendTarget = pipelineDesc.renderState.blendState.targets[i];
			const auto& formatInfo = nvrhi::getFormatInfo(framebufferInfo.colorFormats[i]);

			if (formatInfo.kind == nvrhi::FormatKind::Integer || m_Specification.BlendMode == FramebufferBlendMode::Disabled)
			{
				blendTarget.disableBlend();
				continue;
			}

			blendTarget.enableBlend()
				.setBlendOp(nvrhi::BlendOp::Add)
				.setBlendOpAlpha(nvrhi::BlendOp::Add);

			switch (m_Specification.BlendMode)
			{
				case FramebufferBlendMode::OneZero:
				{
					blendTarget.srcBlend = nvrhi::BlendFactor::One;
					blendTarget.destBlend = nvrhi::BlendFactor::Zero;
					blendTarget.srcBlendAlpha  = nvrhi::BlendFactor::One;
					blendTarget.destBlendAlpha = nvrhi::BlendFactor::Zero;
					break;
				}
				case FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha:
				{
					blendTarget.srcBlend = nvrhi::BlendFactor::SrcAlpha;
					blendTarget.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
					blendTarget.srcBlendAlpha = nvrhi::BlendFactor::SrcAlpha;
					blendTarget.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
					break;
				}
			}
		}

		pipelineDesc.renderState.depthStencilState = nvrhi::DepthStencilState()
			.setDepthTestEnable(m_Specification.DepthEnabled && framebufferInfo.depthFormat != nvrhi::Format::UNKNOWN)
			.setDepthWriteEnable(m_Specification.WriteDepth)
			.setDepthFunc(utils::ConvertOperator(m_Specification.DepthOperator))
			.setStencilEnable(m_Specification.EnableStencil)
			.setStencilRefValue(m_Specification.StencilRef)
			.setStencilReadMask(m_Specification.StencilReadMask)
			.setStencilWriteMask(m_Specification.StencilWriteMask)
			.setBackFaceStencil(stencilOpDesc)
			.setFrontFaceStencil(stencilOpDesc);

		pipelineDesc.renderState.rasterState
			.setCullMode(m_Specification.BackFaceCulling ? nvrhi::RasterCullMode::Back : nvrhi::RasterCullMode::None)
			.setFillMode(m_Specification.WireFrame ? nvrhi::RasterFillMode::Wireframe : nvrhi::RasterFillMode::Solid)
			.setDepthClipEnable(true);

		pipelineDesc.bindingLayouts = m_Specification.Shader->GetBindingLayouts();

		std::vector<nvrhi::VertexAttributeDesc> attributes;
		for (const auto& element : m_Specification.Layout)
		{
			auto desc = nvrhi::VertexAttributeDesc()
				.setName(element.Semantic)
				.setFormat(utils::ConvertVertexDataType(element.Type))
				.setOffset(element.Offset)
				.setElementStride(m_Specification.Layout.GetVertexSize())
				.setIsInstanced(false);

			attributes.push_back(desc);
		}

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		pipelineDesc.inputLayout = device->createInputLayout(attributes.data(), static_cast<uint32_t>(attributes.size()), pipelineDesc.VS);

		m_PipelineHandle = device->createGraphicsPipeline(pipelineDesc, framebufferInfo);
	}

	Pipeline::~Pipeline()
	{

	}

}
