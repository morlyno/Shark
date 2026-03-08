#pragma once

#include "Shark/Render/DeviceManager.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>

#if SK_PLATFORM_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
	#include <vulkan/vulkan_win32.h>
#endif

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
	#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include <vulkan/vulkan.hpp>

namespace Shark {

	struct SurfaceParameters
	{
		WindowHandle Window = nullptr;
		uint32_t Width = 0, Height = 0;
		nvrhi::Format Format = nvrhi::Format::UNKNOWN;
	};

	class VulkanDeviceManager : public DeviceManager
	{
	public:
		virtual Ref<SwapChain> CreateSwapchain(const SwapChainSpecification& specification) override;

		virtual nvrhi::IDevice* GetDevice() const override { return m_NvrhiDevice; }
		nvrhi::vulkan::IDevice* GetNvrhiVulkanDevice() const { return m_NvrhiVulkanDevice; }
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const override { return nvrhi::GraphicsAPI::VULKAN; }

		vk::Instance GetVulkanInstance() const { return m_VulkanInstance; }
		vk::Device GetVulkanDevice() const { return m_VulkanDevice; }
		vk::PhysicalDevice GetVulkanPhysicalDevice() const { return m_VulkanPhysicalDevice; }
		vk::Queue GetPresentQueue() const { return m_PresentQueue; }

		int GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
		int GetPresentQueueFamily() const { return m_PresentQueueFamily; }

		bool GetMutableFormatSupport() const { return m_SwapChainMutableFormatSupported; }

	private:
		virtual void DestroyInternal() override;
		virtual bool CreateInstanceInternal() override;
		virtual bool CreateDeviceInternal() override;
		void InstallDebugCallback();
		bool PickPhysicalDevice(vk::SurfaceKHR surface);
		bool FindQueueFamilies(const vk::PhysicalDevice& device);
		bool CreateVulkanDevice();

		void EnumerateRequiredExtensions();
		void EnumerateRequiredDeviceExtensions();

		vk::SurfaceKHR CreateWindowSurface(WindowHandle window);
		bool WriteSurfaceReport(vk::SurfaceKHR surface, const SurfaceParameters& params, const vk::PhysicalDevice& physicalDevice, fmt::writer& stream) const;

	private:
		nvrhi::vulkan::DeviceHandle m_NvrhiVulkanDevice = nullptr;
		vk::Instance m_VulkanInstance;
		vk::DebugReportCallbackEXT m_DebugReportCallback;

		struct VulkanExtensionSet
		{
			std::unordered_set<std::string> instance;
			std::unordered_set<std::string> layers;
			std::unordered_set<std::string> device;
		};

		// minimal set of required extensions
		VulkanExtensionSet m_EnabledExtensions = {
			// instance
			{
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
			},
			// layers
			{ },
			// device
			{
				VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME,
				VK_GOOGLE_USER_TYPE_EXTENSION_NAME
			},
		};

		// optional extensions
		VulkanExtensionSet m_OptionalExtensions = {
			// instance
			{
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			},
			// layers
			{ },
			// device
			{
				VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
				VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
				VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
				VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
				VK_EXT_MESH_SHADER_EXTENSION_NAME,
				VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
	#if SK_WITH_AFTERMATH
				VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
				VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
	#endif
			},
		};

		vk::PhysicalDevice m_VulkanPhysicalDevice;
		int m_GraphicsQueueFamily = -1;
		int m_ComputeQueueFamily = -1;
		int m_TransferQueueFamily = -1;
		int m_PresentQueueFamily = -1;

		vk::Device m_VulkanDevice;
		vk::Queue m_GraphicsQueue;
		vk::Queue m_ComputeQueue;
		vk::Queue m_TransferQueue;
		vk::Queue m_PresentQueue;

		std::string m_DeviceName;
		bool m_SwapChainMutableFormatSupported = false;
		bool m_BufferDeviceAddressSupported = false;
	};

}
