#pragma once

#include "Shark/Render/GPUTimer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXGPUTimer : public GPUTimer
	{
	public:
		DirectXGPUTimer(const std::string& name);
		virtual ~DirectXGPUTimer();

		virtual TimeStep GetTime() const override { return (float)m_LastTickCount / (float)m_LastFrequency; }

		uint64_t GetTickCount() const { return m_LastTickCount; }
		uint64_t GetFrequency() const { return m_LastFrequency; }

	public:
		void StartQuery(ID3D11DeviceContext* targetContext);
		void EndQuery(ID3D11DeviceContext* targetContext);

		static constexpr uint32_t NumQueries = 5;
	private:
		void NextIndex();
		void UpdateTime();

	private:
		ID3D11Query* m_StartQuery[NumQueries]{};
		ID3D11Query* m_EndQuery[NumQueries]{};
		uint32_t m_Index = 0;
		uint32_t m_DataIndex = 1;
		uint64_t m_LastTickCount = 0;
		uint64_t m_LastFrequency = 0;

		std::string m_Name;

	};

}
