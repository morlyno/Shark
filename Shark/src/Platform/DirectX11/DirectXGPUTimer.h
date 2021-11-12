#pragma once

#include "Shark/Render/GPUTimer.h"


#include <d3d11.h>

namespace Shark {

	class DirectXGPUTimer : public GPUTimer
	{
	public:
		DirectXGPUTimer(const std::string& name);
		virtual ~DirectXGPUTimer();

		virtual TimeStep GetTime() override { return (float)m_LastTime; }

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
		TimeStep m_LastTime = 0.0f;

		std::string m_Name;

	};

}
