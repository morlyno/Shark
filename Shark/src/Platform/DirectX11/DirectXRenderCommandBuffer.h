#pragma once

#include "Shark/Render/RenderCommandBuffer.h"
#include <d3d11_1.h>

namespace Shark {

	class DirectXRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		DirectXRenderCommandBuffer();
		virtual ~DirectXRenderCommandBuffer();

		virtual void Release() override;
		void ReleaseCommandList();

		ID3D11DeviceContext* GetContext() const { return m_Context; }
		ID3DUserDefinedAnnotation* GetAnnotation() const { return m_Annotation; }

		virtual void Begin() override;
		virtual void End() override;
		virtual void Execute() override;

		virtual uint32_t BeginTimestampQuery() override;
		virtual void EndTimestampQuery(uint32_t queryID) override;

		virtual const PipelineStatistics& GetPipelineStatistics() const override { return m_PipelineStatistics; }
		virtual TimeStep GetTime(uint32_t queryID) const override;

		bool IsActive() const { return m_Active; }

	private:
		void CreateQueries();
		void CreateDeferredContext();

	private:
		using TimeQuery = std::pair<ID3D11Query*, ID3D11Query*>;
		using QueryPool = std::vector<TimeQuery>;

		TimeQuery GetNextAvailableTimeQuery(uint32_t& outID);
		TimeQuery GetTimeQuery(uint32_t id);

	private:
		bool m_Active = false;

		ID3D11DeviceContext* m_Context = nullptr;
		ID3DUserDefinedAnnotation* m_Annotation = nullptr;
		ID3D11CommandList* m_CommandList = nullptr;

		std::array<ID3D11Query*, 3> m_PipelineStatsQueries;

		uint32_t m_NextAvailableQueryIndex = 0;
		std::array<uint32_t, 3> m_TimestampQueryCount = { 0, 0, 0 };
		std::array<QueryPool, 3> m_TimestampQueryPools;
		std::array<std::vector<TimeStep>, 3> m_TimestampQueryResults;

		PipelineStatistics m_PipelineStatistics;
	};

}
