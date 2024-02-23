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

	enum class DepthCompareOperator
	{
		Equal, Less, Greater,
		LessEqual, GreaterEqual
	};

	struct PipelineSpecification
	{
		bool BackFaceCulling = false;
		bool WireFrame = false;

		bool DepthEnabled = true;
		bool WriteDepth = true;

		Ref<FrameBuffer> TargetFrameBuffer;
		Ref<Shader> Shader;

		DepthCompareOperator DepthOperator = DepthCompareOperator::LessEqual;
		PrimitveType Primitve = PrimitveType::Triangle;

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
		virtual const PipelineSpecification& GetSpecification() const = 0;

	public:
		static Ref<Pipeline> Create(const PipelineSpecification& specs);

	};

}
