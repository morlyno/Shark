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

}
