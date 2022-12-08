#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/GPUTimer.h"

namespace Shark {

	class RenderCommandBuffer : public RefCount
	{
	public:
		virtual ~RenderCommandBuffer() = default;

		virtual void Release() = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Execute() = 0;

		virtual void BeginTimeQuery(Ref<GPUTimer> counter) = 0;
		virtual void EndTimeQuery(Ref<GPUTimer> counter) = 0;

	public:
		static Ref<RenderCommandBuffer> Create();

	};

}
