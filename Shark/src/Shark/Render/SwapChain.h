#pragma once

#include "Shark/Core/Base.h"
#include "nvrhi/nvrhi.h"

namespace Shark {

	struct SwapChainSpecification
	{
		uint32_t Width, Height;
		uint32_t BufferCount = 0;
		bool Fullscreen = false;
		WindowHandle Window;
	};

	class SwapChain : public RefCount
	{
	public:
		static Ref<SwapChain> Create(const SwapChainSpecification& specification);

		virtual void Present(bool vSync) = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual const nvrhi::FramebufferInfo& GetFramebufferInfo() const = 0;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() = 0;

#if TODO
		virtual void SetFullscreen(bool fullscreen) = 0;

		virtual void AcknowledgeDependency(Weak<FrameBuffer> framebuffer) = 0;
		virtual void AcknowledgeDependency(Weak<Image2D> image) = 0;

		virtual Ref<Image2D> GetTargetImage() const = 0;
		virtual Ref<FrameBuffer> GetFrameBuffer() const = 0;
		virtual const SwapChainSpecification& GetSpecification() const = 0;
#endif
	};

}
