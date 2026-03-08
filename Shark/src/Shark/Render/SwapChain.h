#pragma once

#include "Shark/Core/Base.h"
#include "nvrhi/nvrhi.h"

namespace Shark {

	struct SwapChainSpecification
	{
		uint32_t Width, Height;
		bool Fullscreen = false;
		bool VSync = true;

		WindowHandle Window;
	};

	class SwapChain : public RefCount
	{
	public:
		virtual void BeginFrame() = 0;
		virtual void Present() = 0;
		virtual void WaitForImage() = 0;

		virtual uint32_t GetImageCount() const = 0;
		virtual uint32_t GetCurrentBufferIndex() const = 0;

		virtual nvrhi::ITexture* GetCurrentImage() = 0;
		virtual nvrhi::ITexture* GetImage(uint32_t index) = 0;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() = 0;
		virtual nvrhi::IFramebuffer* GetFramebuffer(uint32_t index) = 0;

		virtual const nvrhi::FramebufferInfo& GetFramebufferInfo() const = 0;

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
