#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/FrameBuffer.h"

namespace Shark {

	struct SwapChainSpecifications
	{
		uint32_t Widht, Height;
		uint32_t BufferCount;
		WindowHandle Handle;
	};

	class SwapChain : public RefCount
	{
	public:
		virtual ~SwapChain() = default;

		virtual void Present(bool vSync) = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual Ref<FrameBuffer> GetFrameBuffer() const = 0;

	public:
		static Ref<SwapChain> Create(const SwapChainSpecifications& specs);

	};

}
