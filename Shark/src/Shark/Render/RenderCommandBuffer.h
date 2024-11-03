#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

namespace Shark {

	struct PipelineStatistics
	{
		uint64_t InputAssemblerVertices;
		uint64_t InputAssemblerPrimitives;
		uint64_t VertexShaderInvocations;
		uint64_t PixelShaderInvocations;
		uint64_t ComputeShaderInvocations;
		uint64_t RasterizerInvocations;
		uint64_t RasterizerPrimitives;
	};

	class RenderCommandBuffer : public RefCount
	{
	public:
		virtual ~RenderCommandBuffer() = default;

		virtual void Release() = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Execute() = 0;

		virtual uint32_t BeginTimestampQuery() = 0;
		virtual void EndTimestampQuery(uint32_t queryID) = 0;

		virtual const PipelineStatistics& GetPipelineStatistics() const = 0;
		virtual TimeStep GetTime(uint32_t queryID) const = 0;

	public:
		static Ref<RenderCommandBuffer> Create();

	};

}
