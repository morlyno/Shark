#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/VertexLayout.h"

namespace Shark {

	enum class PrimitiveType : uint16_t
	{
		Triangle, Line, Dot
	};

	enum class CompareOperator
	{
		Equal, NotEqual, Less, Greater,
		LessEqual, GreaterEqual,
		Never, Always
	};

	enum class StencilOperation
	{
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap
	};

	enum class FramebufferBlendMode
	{
		None = 0,

		OneZero,
		SrcAlphaOneMinusSrcAlpha,

		Disabled = None,
	};

	struct PipelineSpecification
	{
		bool BackFaceCulling = false;
		bool WireFrame = false;

		bool DepthEnabled = true;
		bool WriteDepth = true;

		Ref<Shader> Shader;

		FramebufferBlendMode BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
		CompareOperator DepthOperator = CompareOperator::LessEqual;
		PrimitiveType Primitve = PrimitiveType::Triangle;

		bool EnableStencil = false;
		uint8_t StencilRef = 0xff;
		uint8_t StencilReadMask = 0xff;
		uint8_t StencilWriteMask = 0xff;

		StencilOperation StencilFailOperation = StencilOperation::Keep;
		StencilOperation StencilDepthFailOperation = StencilOperation::Keep;
		StencilOperation StencilPassOperation = StencilOperation::Keep;
		CompareOperator StencilComparisonOperator = CompareOperator::Always;

		VertexLayout Layout;

		std::string DebugName;
	};

	class RenderCommandBuffer;

	class Pipeline : public RefCount
	{
	public:
		static Ref<Pipeline> Create(const PipelineSpecification& specification, const nvrhi::FramebufferInfo& framebufferInfo) { return Ref<Pipeline>::Create(specification, framebufferInfo); }

		nvrhi::GraphicsPipelineHandle GetHandle() const { return m_PipelineHandle; }
		const PipelineSpecification& GetSpecification() const { return m_Specification; }

	public:
		Pipeline(const PipelineSpecification& specification, const nvrhi::FramebufferInfo& framebufferInfo);
		~Pipeline();

	private:
		PipelineSpecification m_Specification;
		nvrhi::GraphicsPipelineHandle m_PipelineHandle;
	};

}
