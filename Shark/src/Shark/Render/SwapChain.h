#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/FrameBuffer.h"

namespace Shark {

	struct SwapChainSpecifications
	{
		uint32_t Width, Height;
		uint32_t BufferCount;
		bool Fullscreen = false;
		WindowHandle Window;
	};

	class SwapChain : public RefCount
	{
	public:
		virtual ~SwapChain() = default;

		virtual void Present(bool vSync) = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void SetFullscreen(bool fullscreen) = 0;

		virtual void AcknowledgeDependency(Weak<FrameBuffer> framebuffer) = 0;
		virtual void AcknowledgeDependency(Weak<Image2D> image) = 0;

		virtual Ref<Image2D> GetTargetImage() const = 0;
		virtual Ref<FrameBuffer> GetFrameBuffer() const = 0;
		virtual const SwapChainSpecifications& GetSpecification() const = 0;

	public:
		static Ref<SwapChain> Create(const SwapChainSpecifications& specs);

	};

}
