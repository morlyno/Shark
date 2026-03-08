#include "skpch.h"
#include "VulkanDeviceManager.h"

#include "Shark/Core/Window.h"
#include "Shark/Utils/std.h"
#include "Shark/Render/Renderer.h"

#if SK_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsUtils.h"
#endif

#include "Shark/Platform/Vulkan/VulkanSwapchain.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Shark {
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		vk::DebugReportFlagsEXT flags,
		vk::DebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData)
	{
		if (flags & vk::DebugReportFlagBitsEXT::eInformation)
		{
			SK_CORE_INFO_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}
		else if (flags & vk::DebugReportFlagBitsEXT::eWarning)
		{
			SK_CORE_WARN_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}
		else if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
		{
			SK_CORE_WARN_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}
		else if (flags & vk::DebugReportFlagBitsEXT::eError)
		{
			SK_CORE_ERROR_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}
		else if (flags & vk::DebugReportFlagBitsEXT::eDebug)
		{
			SK_CORE_DEBUG_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}
		else // Warning and PerformanceWarning
		{
			SK_CORE_WARN_TAG("Renderer", "[Vulkan: location=0x{0:x} code={1}, layerPrefix='{2}'] Frame={4}:\n{3}", location, code, layerPrefix, msg, Renderer::RT_GetCurrentFrameIndex());
		}

		return VK_FALSE;
	}

	Ref<SwapChain> VulkanDeviceManager::CreateSwapchain(const SwapChainSpecification& specification)
	{
		SurfaceParameters params;
		params.Window = specification.Window;
		params.Width = specification.Width;
		params.Height = specification.Height;
		params.Format = m_Specification.SurfaceFormat;

		vk::SurfaceKHR surface = CreateWindowSurface(params.Window);

		fmt::string_buffer report;
		fmt::writer stream(report);

		stream.print("{}:", m_DeviceName);
		if (!WriteSurfaceReport(surface, params, m_VulkanPhysicalDevice, stream))
		{
			SK_CORE_ERROR_TAG("{}", report.str());
			return nullptr;
		}

		return Ref<VulkanSwapChain>::Create(surface, specification);
	}

	void VulkanDeviceManager::DestroyInternal()
	{
		m_VulkanDevice.destroy();
		m_VulkanInstance.destroy();
	}

	bool VulkanDeviceManager::CreateInstanceInternal()
	{
		if (m_Specification.EnableDebugRuntime)
		{
			m_EnabledExtensions.instance.insert(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			m_EnabledExtensions.layers.insert("VK_LAYER_KHRONOS_validation");
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init();

		auto applicationInfo = vk::ApplicationInfo();
		vk::Result result = vk::enumerateInstanceVersion(&applicationInfo.apiVersion);
		if (result != vk::Result::eSuccess)
			return false;

		SK_CORE_VERIFY(applicationInfo.apiVersion >= VK_API_VERSION_1_3);

		EnumerateRequiredExtensions();

		auto instanceExt = m_EnabledExtensions.instance | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();
		auto layers = m_EnabledExtensions.layers | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();

		vk::InstanceCreateInfo createInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExt.size());
		createInfo.ppEnabledExtensionNames = instanceExt.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		createInfo.ppEnabledLayerNames = layers.data();
		createInfo.pApplicationInfo = &applicationInfo;

		#if 0
		uint32_t data[] = { std::numeric_limits<uint32_t>::max() };

		vk::LayerSettingEXT setting;
		setting.pLayerName = "VK_LAYER_KHRONOS_validation";
		setting.pSettingName = "duplicate_message_limit";
		setting.type = vk::LayerSettingTypeEXT::eUint32;
		setting.valueCount = static_cast<uint32_t>(std::size(data));
		setting.pValues = data;

		vk::LayerSettingsCreateInfoEXT settingsCreateInfo;
		settingsCreateInfo.settingCount = 1;
		settingsCreateInfo.pSettings = &setting;
		createInfo.pNext = &settingsCreateInfo;
		#endif

		result = vk::createInstance(&createInfo, nullptr, &m_VulkanInstance);
		if (result != vk::Result::eSuccess)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create vulkan instance! {}", nvrhi::vulkan::resultToString(VkResult(result)));
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanInstance);

		return true;
	}


	bool VulkanDeviceManager::CreateDeviceInternal()
	{
		if (m_Specification.EnableDebugRuntime)
		{
			InstallDebugCallback();
		}

		SK_CORE_VERIFY(m_Specification.Headless == false, "Option not supported at the moment");
		SK_CORE_VERIFY(m_Specification.AllowModeSwitch == false, "Option not supported at the moment");

		{
			// default format and RGB -> BGR
			if (m_Specification.SurfaceFormat == nvrhi::Format::UNKNOWN)
				m_Specification.SurfaceFormat = m_Specification.SrgbSurface ? nvrhi::Format::SBGRA8_UNORM : nvrhi::Format::BGRA8_UNORM;
			else if (m_Specification.SurfaceFormat == nvrhi::Format::SRGBA8_UNORM)
				m_Specification.SurfaceFormat = nvrhi::Format::SBGRA8_UNORM;
			else if (m_Specification.SurfaceFormat == nvrhi::Format::RGBA8_UNORM)
				m_Specification.SurfaceFormat = nvrhi::Format::BGRA8_UNORM;
		}

		vk::SurfaceKHR windowSurface;
		if (!m_Specification.Headless)
		{
			windowSurface = CreateWindowSurface(m_Specification.Window);
		}

		bool success = true;
		success = success && PickPhysicalDevice(windowSurface);
		success = success && FindQueueFamilies(m_VulkanPhysicalDevice);
		success = success && CreateVulkanDevice();

		if (!success)
			return false;

		nvrhi::vulkan::DeviceDesc deviceDesc;
		deviceDesc.errorCB = &MessageCallback::GetInstance();
		deviceDesc.instance = m_VulkanInstance;
		deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
		deviceDesc.device = m_VulkanDevice;
		deviceDesc.graphicsQueue = m_GraphicsQueue;
		deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;

		if (m_Specification.EnableComputeQueue)
		{
			deviceDesc.computeQueue = m_ComputeQueue;
			deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
		}

		if (m_Specification.EnableCopyQueue)
		{
			deviceDesc.transferQueue = m_ComputeQueue;
			deviceDesc.transferQueueIndex = m_TransferQueueFamily;
		}

		std::vector<const char*> instanceExt = m_EnabledExtensions.instance | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();
		std::vector<const char*> deviceExt = m_EnabledExtensions.device | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();
		//std::vector<const char*> layers      = m_EnabledExtensions.layers   | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();

		deviceDesc.instanceExtensions = instanceExt.data();
		deviceDesc.numInstanceExtensions = instanceExt.size();
		deviceDesc.deviceExtensions = deviceExt.data();
		deviceDesc.numDeviceExtensions = deviceExt.size();
		deviceDesc.bufferDeviceAddressSupported = m_BufferDeviceAddressSupported;
		#if SK_WITH_AFTERMATH
		deviceDesc.aftermathEnabled = ...;
		#endif

		m_NvrhiVulkanDevice = nvrhi::vulkan::createDevice(deviceDesc);
		m_NvrhiDevice = m_NvrhiVulkanDevice;

		if (!m_Specification.Headless)
		{
			auto* window = Window::GetFromHandle(m_Specification.Window);

			SwapChainSpecification specification;
			specification.Width = window->GetWidth();
			specification.Height = window->GetHeight();
			specification.Window = m_Specification.Window;
			specification.VSync = window->VSyncEnabled();
			window->SetSwapchain(VulkanSwapChain::Create(windowSurface, specification));
		}

		return true;
	}

	void VulkanDeviceManager::InstallDebugCallback()
{
		auto info = vk::DebugReportCallbackCreateInfoEXT()
			.setFlags(vk::DebugReportFlagBitsEXT::eError |
				vk::DebugReportFlagBitsEXT::eWarning |
				//   vk::DebugReportFlagBitsEXT::eInformation |
				vk::DebugReportFlagBitsEXT::ePerformanceWarning)
			.setPfnCallback(VulkanDebugCallback)
			.setPUserData(this);

		vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT(&info, nullptr, &m_DebugReportCallback);
		assert(res == vk::Result::eSuccess);
		(void)res;
	}

	bool VulkanDeviceManager::PickPhysicalDevice(vk::SurfaceKHR windowSurface)
	{
		auto devices = m_VulkanInstance.enumeratePhysicalDevices();

		int adapterIndex = m_Specification.AdapterIndex;

		int firstDevice = 0;
		int lastDevice = int(devices.size()) - 1;
		if (adapterIndex >= 0)
		{
			if (adapterIndex > lastDevice)
			{
				SK_CORE_ERROR("The specified Vulkan physical device {} does not exist.", adapterIndex);
				return false;
			}
			firstDevice = adapterIndex;
			lastDevice = adapterIndex;
		}

		fmt::string_buffer errorBuffer;
		fmt::writer stream = errorBuffer;

		stream.print("Cannot find a Vulkan device that supports all the required extensions and properties.");

		// build a list of GPUs
		std::vector<vk::PhysicalDevice> discreteGPUs;
		std::vector<vk::PhysicalDevice> otherGPUs;
		for (int deviceIndex = firstDevice; deviceIndex <= lastDevice; ++deviceIndex)
		{
			vk::PhysicalDevice const& dev = devices[deviceIndex];
			vk::PhysicalDeviceProperties prop = dev.getProperties();

			stream.print("\n {}:", prop.deviceName.data());

			bool deviceIsGood = true;

			// check that all required device extensions are present
			std::unordered_set<std::string> requiredExtensions = m_EnabledExtensions.device;
			auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
			for (const auto& ext : deviceExtensions)
			{
				requiredExtensions.erase(std::string(ext.extensionName.data()));
			}

			if (!requiredExtensions.empty())
			{
				// device is missing one or more required extensions
				for (const auto& ext : requiredExtensions)
				{
					stream.print("  - missing {}", ext);
				}
				deviceIsGood = false;
			}

			if (prop.apiVersion < VK_API_VERSION_1_3)
			{
				stream.print("\n  - does not support Vulkan 1.3");
				deviceIsGood = false;
			}

			vk::PhysicalDeviceFeatures2 deviceFeatures2{};
			vk::PhysicalDeviceVulkan13Features vulkan13Features{};
			deviceFeatures2.pNext = &vulkan13Features;

			dev.getFeatures2(&deviceFeatures2);
			if (!deviceFeatures2.features.samplerAnisotropy)
			{
				// device is a toaster oven
				stream.print("\n  - does not support samplerAnisotropy");
				deviceIsGood = false;
			}
			if (!deviceFeatures2.features.textureCompressionBC)
			{
				stream.print("\n  - does not support textureCompressionBC");
				deviceIsGood = false;
			}
			if (!vulkan13Features.dynamicRendering)
			{
				stream.print("\n  - does not support dynamicRendering");
				deviceIsGood = false;
			}
			if (!vulkan13Features.synchronization2)
			{
				stream.print("\n  - does not support synchronization2");
				deviceIsGood = false;
			}


			if (!FindQueueFamilies(dev))
			{
				// device doesn't have all the queue families we need
				stream.print("\n  - does not support the necessary queue types");
				deviceIsGood = false;
			}

			if (deviceIsGood && m_Specification.Window)
			{
				Window* window = Window::GetFromHandle(m_Specification.Window);
				SK_CORE_VERIFY(window);

				SurfaceParameters params;
				params.Width = window->GetWidth();
				params.Height = window->GetHeight();
				params.Format = m_Specification.SurfaceFormat;
				params.Window = m_Specification.Window;

				if (!WriteSurfaceReport(windowSurface, params, dev, stream))
					deviceIsGood = false;
			}

			if (!deviceIsGood)
				continue;

			if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				discreteGPUs.push_back(dev);
			}
			else
			{
				otherGPUs.push_back(dev);
			}
		}

		// pick the first discrete GPU if it exists, otherwise the first integrated GPU
		if (!discreteGPUs.empty())
		{
			uint32_t selectedIndex = 0;
			m_VulkanPhysicalDevice = discreteGPUs[selectedIndex];
			return true;
		}

		if (!otherGPUs.empty())
		{
			uint32_t selectedIndex = 0;
			m_VulkanPhysicalDevice = otherGPUs[selectedIndex];
			return true;
		}

		SK_CORE_ERROR_TAG("Renderer", "{}", errorBuffer.str());

		return false;
	}

	bool VulkanDeviceManager::FindQueueFamilies(const vk::PhysicalDevice& physicalDevice)
	{
		auto props = physicalDevice.getQueueFamilyProperties();

		for (int i = 0; i < int(props.size()); i++)
		{
			const auto& queueFamily = props[i];

			if (m_GraphicsQueueFamily == -1)
			{
				if (queueFamily.queueCount > 0 &&
					(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
				{
					m_GraphicsQueueFamily = i;
				}
			}

			if (m_ComputeQueueFamily == -1)
			{
				if (queueFamily.queueCount > 0 &&
					(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
					!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
				{
					m_ComputeQueueFamily = i;
				}
			}

			if (m_TransferQueueFamily == -1)
			{
				if (queueFamily.queueCount > 0 &&
					(queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
					!(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
					!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
				{
					m_TransferQueueFamily = i;
				}
			}

			if (m_PresentQueueFamily == -1)
			{
				if (queueFamily.queueCount > 0 &&
					physicalDevice.getWin32PresentationSupportKHR(i))
				{
					m_PresentQueueFamily = i;
				}
			}
		}

		if (m_GraphicsQueueFamily == -1 ||
			(m_PresentQueueFamily == -1 && !m_Specification.Headless) ||
			(m_ComputeQueueFamily == -1 && m_Specification.EnableComputeQueue) ||
			(m_TransferQueueFamily == -1 && m_Specification.EnableCopyQueue))
		{
			return false;
		}

		return true;
	}

	bool VulkanDeviceManager::CreateVulkanDevice()
	{
		EnumerateRequiredDeviceExtensions();

		if (!m_Specification.Headless)
		{
			m_EnabledExtensions.device.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		const vk::PhysicalDeviceProperties physicalDeviceProperties = m_VulkanPhysicalDevice.getProperties();
		m_DeviceName = physicalDeviceProperties.deviceName.data();

		bool accelStructSupported = false;
		bool rayPipelineSupported = false;
		bool rayQuerySupported = false;
		bool vrsSupported = false;
		bool interlockSupported = false;
		bool barycentricSupported = false;
		bool aftermathSupported = false;
		bool clusterAccelerationStructureSupported = false;
		bool mutableDescriptorTypeSupported = false;
		bool linearSweptSpheresSupported = false;
		bool meshShaderSupported = false;

		SK_CORE_TRACE_TAG("Renderer", "Enabled Vulkan device extensions:");
		for (const auto& ext : m_EnabledExtensions.device)
		{
			SK_CORE_TRACE_TAG("Renderer", "    {}", ext);

			if (ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
				accelStructSupported = true;
			else if (ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
				rayPipelineSupported = true;
			else if (ext == VK_KHR_RAY_QUERY_EXTENSION_NAME)
				rayQuerySupported = true;
			else if (ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
				vrsSupported = true;
			else if (ext == VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME)
				interlockSupported = true;
			else if (ext == VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)
				barycentricSupported = true;
			else if (ext == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
				m_SwapChainMutableFormatSupported = true;
			else if (ext == VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME)
				aftermathSupported = true;
			else if (ext == VK_NV_CLUSTER_ACCELERATION_STRUCTURE_EXTENSION_NAME)
				clusterAccelerationStructureSupported = true;
			else if (ext == VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME)
				mutableDescriptorTypeSupported = true;
			else if (ext == VK_NV_RAY_TRACING_LINEAR_SWEPT_SPHERES_EXTENSION_NAME)
				linearSweptSpheresSupported = true;
			else if (ext == VK_EXT_MESH_SHADER_EXTENSION_NAME)
				meshShaderSupported = true;
		}


#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
		void* pNext = nullptr;

		vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;

		auto bufferDeviceAddressFeatures = vk::PhysicalDeviceBufferDeviceAddressFeatures();
		auto aftermathPhysicalFeatures = vk::PhysicalDeviceDiagnosticsConfigFeaturesNV();
		auto meshShaderFeatures = vk::PhysicalDeviceMeshShaderFeaturesEXT();

		APPEND_EXTENSION(true, bufferDeviceAddressFeatures);
		APPEND_EXTENSION(aftermathSupported, aftermathPhysicalFeatures);
		APPEND_EXTENSION(meshShaderSupported, meshShaderFeatures);

		physicalDeviceFeatures2.pNext = pNext;
		m_VulkanPhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

		std::unordered_set<int> uniqueQueueFamilies = { m_GraphicsQueueFamily };

		if (!m_Specification.Headless)
			uniqueQueueFamilies.insert(m_PresentQueueFamily);

		if (m_Specification.EnableComputeQueue)
			uniqueQueueFamilies.insert(m_ComputeQueueFamily);

		if (m_Specification.EnableCopyQueue)
			uniqueQueueFamilies.insert(m_TransferQueueFamily);

		float priority = 1.0f;
		std::vector<vk::DeviceQueueCreateInfo> queueDesc;
		queueDesc.reserve(uniqueQueueFamilies.size());
		for (int queueFamily : uniqueQueueFamilies)
		{
			queueDesc.push_back(vk::DeviceQueueCreateInfo()
								.setQueueFamilyIndex(queueFamily)
								.setQueueCount(1)
								.setPQueuePriorities(&priority));
		}

		vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures;
		accelStructFeatures.accelerationStructure = true;

		vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayPipelineFeatures;
		rayPipelineFeatures.rayTracingPipeline = true;
		rayPipelineFeatures.rayTraversalPrimitiveCulling = true;

		vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
		rayQueryFeatures.rayQuery = true;

		vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT interlockFeatures;
		interlockFeatures.fragmentShaderPixelInterlock = true;

		vk::PhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentricFeatures;
		barycentricFeatures.fragmentShaderBarycentric = true;

		vk::PhysicalDeviceFragmentShadingRateFeaturesKHR vrsFeatures;
		vrsFeatures.pipelineFragmentShadingRate = true;
		vrsFeatures.primitiveFragmentShadingRate = true;
		vrsFeatures.attachmentFragmentShadingRate = true;

		vk::PhysicalDeviceVulkan13Features vulkan13features;
		vulkan13features.dynamicRendering = true;
		vulkan13features.synchronization2 = true;
		vulkan13features.maintenance4 = true;
		vulkan13features.shaderDemoteToHelperInvocation = true;


#if SK_WITH_AFTERMATH
		auto aftermathFeatures = vk::DeviceDiagnosticsConfigCreateInfoNV()
			.setFlags(vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking
					  | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo
					  | vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderErrorReporting);
#endif


		vk::PhysicalDeviceClusterAccelerationStructureFeaturesNV clusterAccelerationStructureFeatures;
		clusterAccelerationStructureFeatures.clusterAccelerationStructure = true;

		vk::PhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures;
		mutableDescriptorTypeFeatures.mutableDescriptorType = true;

		vk::PhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV linearSweptSpheresFeatures;
		linearSweptSpheresFeatures.spheres = true;
		linearSweptSpheresFeatures.linearSweptSpheres = true;

		pNext = nullptr;
		APPEND_EXTENSION(true, vulkan13features);
		APPEND_EXTENSION(accelStructSupported, accelStructFeatures);
		APPEND_EXTENSION(rayPipelineSupported, rayPipelineFeatures);
		APPEND_EXTENSION(rayQuerySupported, rayQueryFeatures);
		APPEND_EXTENSION(vrsSupported, vrsFeatures);
		APPEND_EXTENSION(interlockSupported, interlockFeatures);
		APPEND_EXTENSION(barycentricSupported, barycentricFeatures);
		APPEND_EXTENSION(clusterAccelerationStructureSupported, clusterAccelerationStructureFeatures);
		APPEND_EXTENSION(mutableDescriptorTypeSupported, mutableDescriptorTypeFeatures);
		APPEND_EXTENSION(linearSweptSpheresSupported, linearSweptSpheresFeatures);
		APPEND_EXTENSION(meshShaderSupported, meshShaderFeatures);

		// These mesh shader features require other device features to be enabled:
		// - VkPhysicalDeviceMultiviewFeaturesKHR::multiview
		// - VkPhysicalDeviceFragmentShadingRateFeaturesKHR::primitiveFragmentShadingRate
		// Disable the mesh shader features by default, apps can override this if needed.
		meshShaderFeatures.multiviewMeshShader = false;
		meshShaderFeatures.primitiveFragmentShadingRateMeshShader = false;

#if SK_WITH_AFTERMATH
		if (aftermathPhysicalFeatures.diagnosticsConfig && m_DeviceParams.enableAftermath)
			APPEND_EXTENSION(aftermathSupported, aftermathFeatures);
#endif
#undef APPEND_EXTENSION

		vk::PhysicalDeviceFeatures deviceFeatures;
		deviceFeatures.shaderImageGatherExtended = true;
		deviceFeatures.samplerAnisotropy = true;
		deviceFeatures.tessellationShader = true;
		deviceFeatures.textureCompressionBC = true;
		deviceFeatures.geometryShader = true;
		deviceFeatures.imageCubeArray = true;
		deviceFeatures.shaderInt16 = true;
		deviceFeatures.fillModeNonSolid = true;
		deviceFeatures.fragmentStoresAndAtomics = true;
		deviceFeatures.dualSrcBlend = true;
		deviceFeatures.vertexPipelineStoresAndAtomics = true;
		deviceFeatures.shaderInt64 = true;
		deviceFeatures.shaderStorageImageWriteWithoutFormat = true;
		deviceFeatures.shaderStorageImageReadWithoutFormat = true;
		deviceFeatures.independentBlend = true;

		// Add a Vulkan 1.1 structure with default settings to make it easier for apps to modify them
		vk::PhysicalDeviceVulkan11Features vulkan11features;
		vulkan11features.storageBuffer16BitAccess = true;
		vulkan11features.pNext = pNext;

		vk::PhysicalDeviceVulkan12Features vulkan12features;
		vulkan12features.descriptorIndexing = true;
		vulkan12features.runtimeDescriptorArray = true;
		vulkan12features.descriptorBindingPartiallyBound = true;
		vulkan12features.descriptorBindingVariableDescriptorCount = true;
		vulkan12features.timelineSemaphore = true;
		vulkan12features.shaderSampledImageArrayNonUniformIndexing = true;
		vulkan12features.bufferDeviceAddress = bufferDeviceAddressFeatures.bufferDeviceAddress;
		vulkan12features.shaderSubgroupExtendedTypes = true;
		vulkan12features.scalarBlockLayout = true;
		vulkan12features.pNext = &vulkan11features;

		auto extensions = m_EnabledExtensions.device | std::views::transform(std::ranges::data) | std::ranges::to<std::vector>();

		vk::DeviceCreateInfo deviceDesc;
		deviceDesc.pQueueCreateInfos = queueDesc.data();
		deviceDesc.queueCreateInfoCount = static_cast<uint32_t>(queueDesc.size());
		deviceDesc.pEnabledFeatures = &deviceFeatures;
		deviceDesc.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		deviceDesc.ppEnabledExtensionNames = extensions.data();
		deviceDesc.pNext = &vulkan12features;

		const vk::Result res = m_VulkanPhysicalDevice.createDevice(&deviceDesc, nullptr, &m_VulkanDevice);
		if (res != vk::Result::eSuccess)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create a Vulkan physical device, error code = {}", nvrhi::vulkan::resultToString(VkResult(res)));
			return false;
		}

		m_VulkanDevice.getQueue(m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
		if (m_Specification.EnableComputeQueue)
			m_VulkanDevice.getQueue(m_ComputeQueueFamily, 0, &m_ComputeQueue);
		if (m_Specification.EnableCopyQueue)
			m_VulkanDevice.getQueue(m_TransferQueueFamily, 0, &m_TransferQueue);
		if (!m_Specification.Headless)
			m_VulkanDevice.getQueue(m_PresentQueueFamily, 0, &m_PresentQueue);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanDevice);

		// remember the bufferDeviceAddress feature enablement
		m_BufferDeviceAddressSupported = vulkan12features.bufferDeviceAddress;

		SK_CORE_TRACE_TAG("Renderer", "Created Vulkan device: {}", m_DeviceName);

		return true;
	}

	void VulkanDeviceManager::EnumerateRequiredExtensions()
	{
		if (!m_Specification.Headless)
		{
			m_EnabledExtensions.instance.insert(VK_KHR_SURFACE_EXTENSION_NAME);
			m_EnabledExtensions.instance.insert(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		}

		std::unordered_set<std::string> requiredExtensions = m_EnabledExtensions.instance;
		for (const auto& instanceExt : vk::enumerateInstanceExtensionProperties())
		{
			const std::string name = instanceExt.extensionName;
			if (m_OptionalExtensions.instance.find(name) != m_OptionalExtensions.instance.end())
			{
				m_EnabledExtensions.instance.insert(name);
			}

			requiredExtensions.erase(name);
		}

		SK_CORE_VERIFY(requiredExtensions.empty());


		std::unordered_set<std::string> requiredLayers = m_EnabledExtensions.layers;
		for (const auto& layer : vk::enumerateInstanceLayerProperties())
		{
			const std::string name = layer.layerName;
			if (m_OptionalExtensions.layers.find(name) != m_OptionalExtensions.layers.end())
			{
				m_EnabledExtensions.layers.insert(name);
			}

			requiredLayers.erase(name);
		}

		SK_CORE_VERIFY(requiredLayers.empty());
	}

	void VulkanDeviceManager::EnumerateRequiredDeviceExtensions()
	{
		for (const auto& ext : m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties())
		{
			const std::string name = ext.extensionName;
			if (m_OptionalExtensions.device.find(name) != m_OptionalExtensions.device.end())
			{
				if (name == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME && m_Specification.Headless)
					continue;

				m_EnabledExtensions.device.insert(name);
			}

#if 0
			if (m_DeviceParams.enableRayTracingExtensions && m_RayTracingExtensions.find(name) != m_RayTracingExtensions.end())
			{
				enabledExtensions.device.insert(name);
			}
#endif
		}
	}

	vk::SurfaceKHR VulkanDeviceManager::CreateWindowSurface(WindowHandle handle)
	{
		auto hWnd = static_cast<HWND>(handle);
		auto inst = WindowsUtils::GetInstanceFromWindow(hWnd);

		vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.hinstance = inst;
		surfaceCreateInfo.hwnd = hWnd;

		vk::SurfaceKHR surface;
		vk::Result result = m_VulkanInstance.createWin32SurfaceKHR(&surfaceCreateInfo, nullptr, &surface);

		if (result != vk::Result::eSuccess)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create a win32 window wurface, error code: {}", nvrhi::vulkan::resultToString(VkResult(result)));
			return nullptr;
		}

		return surface;
	}

	bool VulkanDeviceManager::WriteSurfaceReport(vk::SurfaceKHR windowSurface, const SurfaceParameters& params, const vk::PhysicalDevice& physicalDevice, fmt::writer& stream) const
	{
		if (!physicalDevice.getSurfaceSupportKHR(m_PresentQueueFamily, windowSurface))
		{
			stream.print("\n  - does not support the window surface");
			return false;
		}

		bool deviceIsGood = true;

		// check that this device supports our intended swap chain creation parameters
		auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(windowSurface);
		auto surfaceFmts = physicalDevice.getSurfaceFormatsKHR(windowSurface);

		if (surfaceCaps.minImageCount > m_Specification.SwapchainBufferCount ||
			(surfaceCaps.maxImageCount < m_Specification.SwapchainBufferCount && surfaceCaps.maxImageCount > 0))
		{
			stream.print("\n");
			stream.print("  - cannot support the requested swap chain image count: requested {}, available {} - {}", m_Specification.SwapchainBufferCount, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);

			deviceIsGood = false;
		}

		if (surfaceCaps.minImageExtent.width > params.Width ||
			surfaceCaps.minImageExtent.height > params.Height ||
			surfaceCaps.maxImageExtent.width < params.Width ||
			surfaceCaps.maxImageExtent.height < params.Height)
		{
			stream.print("\n");
			stream.print("  - cannot support the requested swap chain size: requested {} x {}, available {} x {} - {} x {}",
						 params.Width, params.Height,
						 surfaceCaps.minImageExtent.width, surfaceCaps.minImageExtent.height,
						 surfaceCaps.maxImageExtent.width, surfaceCaps.maxImageExtent.height);
			deviceIsGood = false;
		}

		bool surfaceFormatPresent = false;
		for (const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts)
		{
			if (surfaceFmt.format == vk::Format(nvrhi::vulkan::convertFormat(params.Format)))
			{
				surfaceFormatPresent = true;
				break;
			}
		}

		if (!surfaceFormatPresent)
		{
			// can't create a swap chain using the format requested
			stream.print("\n  - does not support the requested swap chain format");
			deviceIsGood = false;
		}

		// check that we can present from the graphics queue
		uint32_t canPresent = physicalDevice.getSurfaceSupportKHR(m_GraphicsQueueFamily, windowSurface);
		if (!canPresent)
		{
			stream.print("\n  - cannot present");
			deviceIsGood = false;
		}

		return deviceIsGood;
	}

}
