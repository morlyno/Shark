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
		virtual void Execute(Ref<GPUPipelineQuery> query) = 0;

		virtual void BeginQuery(Ref<GPUTimer> query) = 0;
		virtual void BeginQuery(Ref<GPUPipelineQuery> query) = 0;

		virtual void EndQuery(Ref<GPUTimer> query) = 0;
		virtual void EndQuery(Ref<GPUPipelineQuery> query) = 0;

	public:
		static Ref<RenderCommandBuffer> Create();

	};

}
