#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Platform/Vulkan/VulkanDeviceManager.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>

namespace Shark {

	class VulkanSwapChain : public SwapChain
	{
	public:
		static Ref<VulkanSwapChain> Create(vk::SurfaceKHR surface, const SwapChainSpecification& specification) { return Ref<VulkanSwapChain>::Create(surface, specification); }

		VulkanSwapChain(vk::SurfaceKHR surface, const SwapChainSpecification& specification);
		~VulkanSwapChain();

		virtual void BeginFrame() override;
		virtual void Present() override;
		virtual void WaitForImage() override;

		virtual uint32_t GetImageCount() const { return m_ImageCount; }
		virtual uint32_t GetCurrentBufferIndex() const { return m_ImageIndex; }

		virtual nvrhi::ITexture* GetCurrentImage() override;
		virtual nvrhi::ITexture* GetImage(uint32_t index) override;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() override;
		virtual nvrhi::IFramebuffer* GetFramebuffer(uint32_t index) override;

		virtual const nvrhi::FramebufferInfo& GetFramebufferInfo() const override { return m_FramebufferInfo; };

	private:
		void Release();
		void DestroySwapchain();

		void CreateSwapchain();
		void CreateRenderTarget();
		void CreateSemaphores();

	private:
		SwapChainSpecification m_Specification;

		vk::SurfaceKHR m_WindowSurface;

		vk::SurfaceFormatKHR m_SwapChainFormat;
		vk::SwapchainKHR m_SwapChain;

		std::vector<vk::Image> m_VulkanImages;
		std::vector<nvrhi::TextureHandle> m_NvrhiTextures;
		std::vector<nvrhi::FramebufferHandle> m_Framebuffers;

		vk::Semaphore m_ActiveSemaphore;
		std::vector<vk::Semaphore> m_AcquireSemaphores;
		std::vector<vk::Semaphore> m_PresentSemaphores;

		std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
		std::vector<nvrhi::EventQueryHandle> m_QueryPool;

		uint32_t m_AcquireIndex = 0;

		uint32_t m_ImageCount = 0;
		uint32_t m_ImageIndex = 0;

		nvrhi::FramebufferInfo m_FramebufferInfo;
	};

}
