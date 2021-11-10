#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class GPUTimer : public RefCount
	{
	public:
		virtual ~GPUTimer() = default;

		virtual float GetTime() = 0;

	public:
		static Ref<GPUTimer> Create(const std::string& name = std::string{});
	};

}
