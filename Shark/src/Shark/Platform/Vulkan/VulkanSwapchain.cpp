#include "skpch.h"
#include "VulkanSwapchain.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	VulkanSwapChain::VulkanSwapChain(vk::SurfaceKHR surface, const SwapChainSpecification& specification)
		: m_Specification(specification), m_WindowSurface(surface)
	{
		CreateSwapchain();
		CreateSemaphores();
		CreateRenderTarget();

		m_FramebufferInfo = static_cast<const nvrhi::FramebufferInfo&>(m_Framebuffers[0]->getFramebufferInfo());
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		Release();
	}

	void VulkanSwapChain::BeginFrame()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Swapchain - BeginFrame");

		auto deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		auto vulkanDevice = deviceManager->GetVulkanDevice();
		auto physicalDevice = deviceManager->GetVulkanPhysicalDevice();

		const auto& semaphore = m_AcquireSemaphores[m_AcquireIndex];
		vk::Result result;

		const uint32_t maxAttempts = 3;
		for (uint32_t attemp = 0; attemp < maxAttempts; attemp++)
		{
			result = vulkanDevice.acquireNextImageKHR(
				m_SwapChain,
				std::numeric_limits<uint64_t>::max(),
				semaphore,
				vk::Fence(),
				&m_ImageIndex
			);

			if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
			{
				auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(m_WindowSurface);
				m_Specification.Width = surfaceCaps.currentExtent.width;
				m_Specification.Height = surfaceCaps.currentExtent.height;

				SK_CORE_WARN_TAG("Renderer", "[{}] AcquireNextImage returned {}, Recreating swap chain", Renderer::RT_GetCurrentFrameIndex(), nvrhi::vulkan::resultToString(VkResult(result)));
				DestroySwapchain();
				CreateSwapchain();
				CreateRenderTarget();
				continue;
			}

			break;
		}

		m_ActiveSemaphore = semaphore;
		++m_AcquireIndex %= m_AcquireSemaphores.size();

		if (!(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR))
		{
			SK_CORE_ERROR_TAG("Renderer", "[{}] Failed to acquire image! {}", Renderer::RT_GetCurrentFrameIndex(), nvrhi::vulkan::resultToString(VkResult(result)));
			return;
		}
	}

	void VulkanSwapChain::Present()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Swapchain - Present");

		auto deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		auto device = deviceManager->GetNvrhiVulkanDevice();

		const auto& semaphore = m_PresentSemaphores[m_ImageIndex];

		vk::PresentInfoKHR presentInfo;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &m_ImageIndex;

		vk::Result result;

		{
			deviceManager->LockQueue();

			device->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);
			device->executeCommandLists(nullptr, 0);

			auto presentQueue = deviceManager->GetPresentQueue();
			result = presentQueue.presentKHR(&presentInfo);
			deviceManager->UnlockQueue();
		}

		if (result != vk::Result::eSuccess && result != vk::Result::eErrorOutOfDateKHR && result != vk::Result::eSuboptimalKHR)
		{
			SK_CORE_ERROR_TAG("Renderer", "[{}] Failed to present! {}", Renderer::RT_GetCurrentFrameIndex(), nvrhi::vulkan::resultToString(VkResult(result)));
			return;
		}

#if !SK_PLATFORM_WINDOWS
		if (vSync || deviceManager->GetSpecification().EnableDebugRuntime)
			presentQueue.waitIdle();
#endif

		while (m_FramesInFlight.size() >= deviceManager->GetSpecification().MaxFramesInFlight)
		{
			SK_PERF_SCOPED("Frames Wait");

			auto query = m_FramesInFlight.front();
			m_FramesInFlight.pop();

			device->waitEventQuery(query);
			m_QueryPool.push_back(query);
		}

		nvrhi::EventQueryHandle query;
		if (!m_QueryPool.empty())
		{
			query = m_QueryPool.back();
			m_QueryPool.pop_back();
		}
		else
		{
			query = device->createEventQuery();
		}

		device->resetEventQuery(query);
		device->setEventQuery(query, nvrhi::CommandQueue::Graphics);
		m_FramesInFlight.push(query);
	}

	void VulkanSwapChain::WaitForImage()
	{
		auto deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		auto device = deviceManager->GetNvrhiVulkanDevice();

		deviceManager->LockQueue();
		device->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, m_ActiveSemaphore, 0);
		deviceManager->UnlockQueue();
	}

	nvrhi::ITexture* VulkanSwapChain::GetCurrentImage()
	{
		return GetImage(m_ImageIndex);
	}

	nvrhi::ITexture* VulkanSwapChain::GetImage(uint32_t index)
	{
		if (index >= m_NvrhiTextures.size())
			return nullptr;

		return m_NvrhiTextures[index];
	}

	nvrhi::IFramebuffer* VulkanSwapChain::GetCurrentFramebuffer()
	{
		return GetFramebuffer(m_ImageIndex);
	}

	nvrhi::IFramebuffer* VulkanSwapChain::GetFramebuffer(uint32_t index)
	{
		if (index >= m_Framebuffers.size())
			return nullptr;

		return m_Framebuffers[index];
	}

	void VulkanSwapChain::Release()
	{
		auto deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		auto vulkanDevice = deviceManager->GetVulkanDevice();

		vulkanDevice.waitIdle();

		if (m_SwapChain)
		{
			vulkanDevice.destroySwapchainKHR(m_SwapChain);
			m_SwapChain = nullptr;
		}

		for (const auto& semaphore : m_AcquireSemaphores)
			vulkanDevice.destroySemaphore(semaphore);

		for (const auto& semaphore : m_PresentSemaphores)
			vulkanDevice.destroySemaphore(semaphore);

		m_NvrhiTextures.clear();
		m_VulkanImages.clear();
		m_Framebuffers.clear();

		auto instance = deviceManager->GetVulkanInstance();
		instance.destroySurfaceKHR(m_WindowSurface);
	}

	void VulkanSwapChain::DestroySwapchain()
	{
		auto vulkanDevice = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager())->GetVulkanDevice();

		vulkanDevice.waitIdle();
		vulkanDevice.destroySwapchainKHR(m_SwapChain);
		m_SwapChain = nullptr;

		m_VulkanImages.clear();
		m_NvrhiTextures.clear();
	}

	void VulkanSwapChain::CreateSwapchain()
	{
		auto* deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		const auto& deviceSpecification = deviceManager->GetSpecification();

		const nvrhi::Format format = deviceSpecification.SurfaceFormat;

		m_SwapChainFormat = {
			vk::Format(nvrhi::vulkan::convertFormat(format)),
			vk::ColorSpaceKHR::eSrgbNonlinear
		};

		vk::Extent2D extent = vk::Extent2D(m_Specification.Width, m_Specification.Height);

		std::vector<uint32_t> queues = {
			uint32_t(deviceManager->GetGraphicsQueueFamily()),
			uint32_t(deviceManager->GetPresentQueueFamily())
		};

		const auto ret = std::ranges::unique(queues);
		queues.erase(ret.begin(), ret.end());
		std::ranges::sort(queues);

		const bool enableSwapChainSharing = queues.size() > 1;
		const bool mutableFormatSupport = deviceManager->GetMutableFormatSupport();

		vk::SwapchainCreateInfoKHR desc;
		desc.surface = m_WindowSurface;
		desc.minImageCount = deviceSpecification.SwapchainBufferCount;
		desc.imageFormat = m_SwapChainFormat.format;
		desc.imageColorSpace = m_SwapChainFormat.colorSpace;
		desc.imageExtent = extent;
		desc.imageArrayLayers = 1;
		desc.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		desc.imageSharingMode = enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
		desc.flags = mutableFormatSupport ? vk::SwapchainCreateFlagBitsKHR::eMutableFormat : vk::SwapchainCreateFlagBitsKHR(0);
		desc.queueFamilyIndexCount = enableSwapChainSharing ? static_cast<uint32_t>(queues.size()) : 0;
		desc.pQueueFamilyIndices = enableSwapChainSharing ? queues.data() : nullptr;
		desc.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		desc.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		desc.presentMode = m_Specification.VSync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
		desc.clipped = true;
		desc.oldSwapchain = nullptr;

		std::vector<vk::Format> imageFormats = { m_SwapChainFormat.format };
		switch (m_SwapChainFormat.format)
		{
			case vk::Format::eR8G8B8A8Unorm:
				imageFormats.push_back(vk::Format::eR8G8B8A8Srgb);
				break;
			case vk::Format::eR8G8B8A8Srgb:
				imageFormats.push_back(vk::Format::eR8G8B8A8Unorm);
				break;
			case vk::Format::eB8G8R8A8Unorm:
				imageFormats.push_back(vk::Format::eB8G8R8A8Srgb);
				break;
			case vk::Format::eB8G8R8A8Srgb:
				imageFormats.push_back(vk::Format::eB8G8R8A8Unorm);
				break;
		}

		vk::ImageFormatListCreateInfo formatListCreateInfo;
		formatListCreateInfo.setViewFormats(imageFormats);

		if (mutableFormatSupport)
			desc.pNext = &formatListCreateInfo;

		auto device = deviceManager->GetDevice();
		auto vulkanDevice = deviceManager->GetVulkanDevice();

		vk::Result result = vulkanDevice.createSwapchainKHR(&desc, nullptr, &m_SwapChain);
		if (result != vk::Result::eSuccess)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create Vulkan swap chain! {}", nvrhi::vulkan::resultToString(VkResult(result)));
			return;
		}

		auto images = vulkanDevice.getSwapchainImagesKHR(m_SwapChain);
		for (auto image : images)
		{
			nvrhi::TextureDesc textureDesc;
			textureDesc.width = m_Specification.Width;
			textureDesc.height = m_Specification.Height;
			textureDesc.format = format;
			textureDesc.debugName = "SwapchainImage";
			textureDesc.enableAutomaticStateTracking(nvrhi::ResourceStates::Present);
			textureDesc.isRenderTarget = true;

			auto texture = device->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(image), textureDesc);

			m_VulkanImages.push_back(image);
			m_NvrhiTextures.push_back(texture);
		}

		m_ImageCount = m_VulkanImages.size();
		m_ImageIndex = 0;

		SK_CORE_INFO_TAG("Renderer", "Create swap chain with {} images", m_ImageCount);
	}

	void VulkanSwapChain::CreateRenderTarget()
	{
		auto* device = Renderer::GetGraphicsDevice();

		m_Framebuffers.clear();
		m_Framebuffers.resize(m_ImageCount);

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			nvrhi::FramebufferDesc desc;
			desc.addColorAttachment(m_NvrhiTextures[i]);

			m_Framebuffers[i] = device->createFramebuffer(desc);
		}
	}

	void VulkanSwapChain::CreateSemaphores()
	{
		auto* deviceManager = static_cast<VulkanDeviceManager*>(Renderer::GetDeviceManager());
		const auto& deviceSpecification = deviceManager->GetSpecification();

		auto vulkanDevice = deviceManager->GetVulkanDevice();

		m_AcquireSemaphores.resize(std::max(deviceSpecification.MaxFramesInFlight, m_ImageCount));
		m_PresentSemaphores.resize(m_ImageCount);

		for (uint32_t i = 0; i < m_AcquireSemaphores.size(); i++)
		{
			m_AcquireSemaphores[i] = vulkanDevice.createSemaphore(vk::SemaphoreCreateInfo());
		}

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			m_PresentSemaphores[i] = vulkanDevice.createSemaphore(vk::SemaphoreCreateInfo());
		}
	}

}
