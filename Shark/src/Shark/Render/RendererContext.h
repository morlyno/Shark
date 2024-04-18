#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class RendererContext : public RefCount
	{
	public:
		static Ref<RendererContext> Create();

		virtual void ReportLiveObjects() = 0;
	};

}
