#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	struct SwapChainSpecifications
	{
		uint32_t Widht;
		uint32_t Height;
		void* WindowHandle;

		uint32_t BufferCount = 1;
	};

	class SwapChain : public RefCount
	{
	public:
		virtual ~SwapChain() = default;

		virtual void SwapBuffers(bool vsync) = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetBufferCount() const = 0;

		static Ref<SwapChain> Create(const SwapChainSpecifications& specs);
	};

}
