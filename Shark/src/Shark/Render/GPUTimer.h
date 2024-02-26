#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

namespace Shark {

	class GPUTimer : public RefCount
	{
	public:
		virtual ~GPUTimer() = default;

		virtual TimeStep GetTime() const = 0;

	public:
		static Ref<GPUTimer> Create(const std::string& name = std::string{});
	};

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

	class GPUPipelineQuery : public RefCount
	{
	public:
		virtual const PipelineStatistics& GetStatistics() const = 0;

	public:
		static Ref<GPUPipelineQuery> Create(const std::string& name);
	};

}
