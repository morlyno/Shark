#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class RenderCommandBuffer : public RefCount
	{
	public:
		virtual ~RenderCommandBuffer() = default;

		virtual void Begin(bool clearState = false) = 0;
		virtual void End() = 0;
		virtual void Execute() = 0;

		static Ref<RenderCommandBuffer> Create();

	};

}
