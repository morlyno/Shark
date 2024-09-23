#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/VertexLayout.h"

namespace Shark {

	enum class PrimitveType : uint16_t
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

	struct PipelineSpecification
	{
		bool BackFaceCulling = false;
		bool WireFrame = false;

		bool DepthEnabled = true;
		bool WriteDepth = true;

		Ref<FrameBuffer> TargetFrameBuffer;
		Ref<Shader> Shader;

		CompareOperator DepthOperator = CompareOperator::LessEqual;
		PrimitveType Primitve = PrimitveType::Triangle;

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

	class Pipeline : public RefCount
	{
	public:
		virtual ~Pipeline() = default;

		virtual void SetPushConstant(Buffer pushConstantData) = 0;

		template<typename T>
		void SetPushConstant(const T& value)
		{
			SetPushConstant(Buffer::FromValue(value));
		}

		virtual void SetFrameBuffer(Ref<FrameBuffer> frameBuffer) = 0;
		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

	public:
		static Ref<Pipeline> Create(const PipelineSpecification& specs);

	};

}
