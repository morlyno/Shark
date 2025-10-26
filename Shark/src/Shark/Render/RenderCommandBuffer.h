#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <nvrhi/nvrhi.h>

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

	class QueryID
	{
	public:
		QueryID() = default;

		bool IsValid() const { return m_ID != Invalid; }

	private:
		QueryID(uint32_t id) : m_ID(id) {}

		operator uint32_t() const { return m_ID; }
		static constexpr uint32_t Invalid = (uint32_t)-1;

	private:
		uint32_t m_ID = Invalid;

		friend class RenderCommandBuffer;
	};

	class RenderCommandBuffer : public RefCount
	{
	public:
		static Ref<RenderCommandBuffer> Create(const std::string& name, uint32_t queryCountHint = 0) { return Ref<RenderCommandBuffer>::Create(name, queryCountHint); }

	public:
		void Begin();
		void End();
		void Execute();

		void RT_Begin();
		void RT_End();
		void RT_Execute();

		nvrhi::CommandListHandle GetHandle() const { return m_CommandList; }
		nvrhi::GraphicsState& GetGraphicsState() { return m_GraphicsState; }

		void BeginMarker(const char* name);
		void EndMarker();

		QueryID RegisterTimerQuery();
		void BeginTimerQuery(QueryID queryID);
		void EndTimerQuery(QueryID queryID);
		void RT_BeginTimerQuery(QueryID queryID);
		void RT_EndTimerQuery(QueryID queryID);

		TimeStep GetGPUExecutionTime(QueryID queryID = QueryID(0)) const;
		TimeStep GetGPUExecutionTime(uint32_t frameIndex, QueryID queryID = QueryID(0)) const;

		const PipelineStatistics& GetPipelineStatistics() const { return PipelineStatistics{}; }

	public:
		RenderCommandBuffer(const std::string& name, uint32_t queryCountHint);
		~RenderCommandBuffer();

	private:
		std::string m_Name;
		nvrhi::CommandListHandle m_CommandList;
		nvrhi::GraphicsState m_GraphicsState;

		QueryID m_TimerQuery;
		uint32_t m_NextQueryID = 0;
		std::vector<std::vector<std::pair<nvrhi::TimerQueryHandle, bool>>> m_TimerQueryPools;
		std::vector<std::vector<float>> m_GPUExecutionTimes;
	};

}
