#pragma once

#include <random>

namespace Shark {

	class Random
	{
	public:
		static float Float()
		{
			return Get().m_RandomDistribution(Get().m_RngDevice);
		}
	private:
		static Random& Get()
		{
			static Random s_Inst;
			return s_Inst;
		}

		std::uniform_real_distribution<float> m_RandomDistribution;
		std::random_device m_RngDevice;
	};

}
