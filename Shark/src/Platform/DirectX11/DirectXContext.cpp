#include "skpch.h"
#include "DirectXContext.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/Windows/WindowsUtils.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include <dxgi1_3.h>

namespace Shark {

#define INTERNAL_GET_FIRST(_macro, ...) _macro

#define DX_DESTROYED_VERIFY(_release_call, _ref_count) SK_CORE_VERIFY(false, "Object was not destroyed! Refcount is {} " #_release_call " {}", _ref_count, std::source_location::current())
#define DX_DESTROYED_LOG_ERROR(_release_call, _ref_count) SK_CORE_ERROR("Object was not destroyed! Refcount is {} " #_release_call " {}", _ref_count, std::source_location::current())

#define DX_DESTROYED_DEFAULT(_release_call, _ref_count) DX_DESTROYED_LOG_ERROR(_release_call, _ref_count); SK_DEBUG_BREAK()
#define DX_DESTROYED(_release_call, ...) { const ULONG count = (_release_call); while (count != 0) { __VA_OPT__(INTERNAL_GET_FIRST(__VA_ARGS__)(_release_call, count); break;); DX_DESTROYED_DEFAULT(_release_call, count); break; } }

	namespace utils {

		static std::string GetGPUDescription(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad = {};
			DX11_VERIFY(adapter->GetDesc(&ad));
			return String::ToNarrow(std::wstring_view(ad.Description));
		}

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Context //////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DirectXContext::DirectXContext()
	{
		m_MessageThread = Threading::Thread("DX Message Thread", [this](std::stop_token stopToken) { MessageThreadFunc(std::move(stopToken)); });
		m_Device = Ref<DirectXDevice>::Create();
	}

	DirectXContext::~DirectXContext()
	{
		DestroyDevice();

		if (m_MessageThread.Running())
		{
			m_MessageThread.RequestStop();
			m_MessageSignal.Notify();
			m_MessageThread.Join();
		}
	}

	void DirectXContext::DestroyDevice()
	{
		if (!m_Device)
			return;

		m_Device = nullptr;
		SK_CORE_WARN_TAG("Renderer", "DirectXContext destroyed");
	}

	void DirectXContext::HandleError(HRESULT hr)
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG || hr == DXGI_ERROR_DEVICE_RESET)
		{
			HRESULT deviceRemovedHR = m_Device->GetDirectXDevice()->GetDeviceRemovedReason();
			auto deviceRemovedReason = WindowsUtils::TranslateHResult(deviceRemovedHR);
			SK_CORE_CRITICAL_TAG("Renderer", deviceRemovedReason);
			return;
		}

		m_MessageSignal.Notify();

		static bool BreakOnError = true;
		if (BreakOnError)
		{
			SK_DEBUG_BREAK();
		}

	}

	void DirectXContext::ReportLiveObjects()
	{
		IDXGIDebug1* debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
			m_MessageSignal.Notify();
			debug->Release();
		}
	}

	namespace utils {

		static std::string_view GetCategoryName(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category)
		{
			switch (category)
			{
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN: return "Unknown";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS: return "Miscellaneous";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION: return "Initialization";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP: return "Cleanup";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION: return "Compilation";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION: return "State Creation";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING: return "State Setting";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING: return "State Getting";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: return "ResourceManipulation";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION: return "Execution";
				case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER: return "Shader";
			}
			return "Unknown";
		}

	}

	void DirectXContext::MessageThreadFunc(std::stop_token stopToken)
	{
		const auto MessageCallback = [](DXGI_DEBUG_ID producer, DXGI_INFO_QUEUE_MESSAGE_CATEGORY category, DXGI_INFO_QUEUE_MESSAGE_SEVERITY level, std::string_view message)
		{
			std::string_view producerName;
			if (producer == DXGI_DEBUG_ALL)
				producerName = "All";
			else if (producer == DXGI_DEBUG_DX)
				producerName = "DX";
			else if (producer == DXGI_DEBUG_DXGI)
				producerName = "DXGI";
			else if (producer == DXGI_DEBUG_APP)
				producerName = "APP";

			const std::string_view categoryName = utils::GetCategoryName(category);
			switch (level)
			{
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: SK_CORE_CRITICAL_TAG("Renderer", "{} {}\n{}", producerName, categoryName, message); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR: SK_CORE_ERROR_TAG("Renderer", "{} {}\n{}", producerName, categoryName, message); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: SK_CORE_WARN_TAG("Renderer", "{} {}\n{}", producerName, categoryName, message); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO: SK_CORE_INFO_TAG("Renderer", "{} {}\n{}", producerName, categoryName, message); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE: SK_CORE_TRACE_TAG("Renderer", "{} {}\n{}", producerName, categoryName, message); break;
			}
		};

		IDXGIInfoQueue* infoQueue = nullptr;
		if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&infoQueue))))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to get the DXGI info queue! Stopping message thread.");
			return;
		}

		const DXGI_DEBUG_ID producer = DXGI_DEBUG_ALL;

		infoQueue->PushEmptyRetrievalFilter(producer);
		infoQueue->PushEmptyStorageFilter(producer);

		Buffer messageBuffer;
		uint64_t byteLength = 0;
		using MESSAGE_TYPE = DXGI_INFO_QUEUE_MESSAGE;

		while (!stopToken.stop_requested())
		{
			const UINT64 count = infoQueue->GetNumStoredMessages(producer);
			for (uint64_t i = 0; i < count; i++)
			{
				infoQueue->GetMessage(producer, i, nullptr, &byteLength);

				messageBuffer.Resize(byteLength, false);
				auto message = messageBuffer.As<MESSAGE_TYPE>();

				infoQueue->GetMessage(producer, i, message, &byteLength);

				MessageCallback(message->Producer, message->Category, message->Severity, { message->pDescription, message->DescriptionByteLength });
			}
			infoQueue->ClearStoredMessages(producer);

			m_MessageSignal.Wait(100ms);
		}

		messageBuffer.Release();
		infoQueue->Release();
		SK_CORE_WARN_TAG("Renderer", "Message Thread Terminated");
	}

	Ref<DirectXDevice> DirectXContext::GetCurrentDevice()
	{
		return Renderer::GetRendererContext().As<DirectXContext>()->m_Device;
	}

	Ref<DirectXContext> DirectXContext::Get()
	{
		return Renderer::GetRendererContext().As<DirectXContext>();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Device ///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DirectXDevice::DirectXDevice()
	{
		UINT factoryFlags = SK_ENABLE_GPU_VALIDATION ? DXGI_CREATE_FACTORY_DEBUG : 0;

		DX11_VERIFY(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* adapter = nullptr;
		DX11_VERIFY(m_Factory->EnumAdapters(0, &adapter));
		SK_CORE_INFO_TAG("Renderer", "GPU Selected ({0})", utils::GetGPUDescription(adapter));

		UINT createdeviceFalgs = 0u;
#if SK_ENABLE_GPU_VALIDATION
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		ID3D11Device* device;
		DX11_VERIFY(D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&device,
			nullptr,
			&m_Queue
		));

		adapter->Release();

		DX11_VERIFY(device->QueryInterface(IID_PPV_ARGS(&m_Device)));
		device->Release();

		{
			ID3D11Query* query = nullptr;
			DirectXAPI::CreateQuery(m_Device, D3D11_QUERY_TIMESTAMP_DISJOINT, query);

			m_Queue->Begin(query);
			m_Queue->End(query);

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
			m_Queue->GetData(query, &data, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
			m_Limits.TimestampPeriod = data.Frequency;

			query->Release();
		}
	}

	DirectXDevice::~DirectXDevice()
	{
		m_CommandPools.clear();

		m_Factory->Release();
		DX_DESTROYED(m_Queue->Release(), DX_DESTROYED_LOG_ERROR);
		DX_DESTROYED(m_Device->Release(), DX_DESTROYED_LOG_ERROR);

		SK_CORE_WARN_TAG("Renderer", "DirectXDevice destroyed");
	}

	void DirectXDevice::MapMemory(ID3D11Resource* resource, uint32_t subresource, D3D11_MAP mapFlags, void*& outMemory)
	{
		SK_PROFILE_FUNCTION();
		std::scoped_lock lock(m_SubmissionMutex);

		D3D11_MAPPED_SUBRESOURCE mapped;
		m_Queue->Map(resource, subresource, mapFlags, 0, &mapped);
		outMemory = mapped.pData;
	}

	void DirectXDevice::MapMemory(ID3D11Resource* resource, uint32_t subresource, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE& outMapped)
	{
		SK_PROFILE_FUNCTION();
		std::scoped_lock lock(m_SubmissionMutex);
		m_Queue->Map(resource, subresource, mapType, 0, &outMapped);
	}

	void DirectXDevice::UnmapMemory(ID3D11Resource* resource, uint32_t subresource)
	{
		SK_PROFILE_FUNCTION();
		std::scoped_lock lock(m_SubmissionMutex);
		m_Queue->Unmap(resource, subresource);
	}

	void DirectXDevice::UpdateSubresource(ID3D11Resource* destination, uint32_t destSubresource, const D3D11_BOX* destBox, const void* sourceData, uint32_t sourceRowPitch, uint32_t sourceDepthPitch)
	{
		SK_PROFILE_FUNCTION();
		std::scoped_lock lock(m_SubmissionMutex);
		m_Queue->UpdateSubresource(destination, destSubresource, destBox, sourceData, sourceRowPitch, sourceDepthPitch);
	}

	void DirectXDevice::CopySubresource(ID3D11Resource* destination, uint32_t destSubresource, uint32_t destX, uint32_t dextY, uint32_t destZ, ID3D11Resource* source, uint32_t srcSubresource, const D3D11_BOX* srcBox)
	{
		SK_PROFILE_FUNCTION();
		std::scoped_lock lock(m_SubmissionMutex);
		m_Queue->CopySubresourceRegion(destination, destSubresource, destX, dextY, destZ, source, srcSubresource, srcBox);
	}

	ID3D11DeviceContext* DirectXDevice::AllocateCommandBuffer()
	{
		return GetOrCreateThreadLocalCommandPool()->AllocateCommandBuffer();
	}

	void DirectXDevice::FlushCommandBuffer(ID3D11DeviceContext* commandBuffer)
	{
		GetOrCreateThreadLocalCommandPool()->FlushCommandBuffer(commandBuffer);
	}

	Ref<DirectXCommandPool> DirectXDevice::GetThreadLocalCommandPool()
	{
		SK_CORE_VERIFY(m_CommandPools.contains(std::this_thread::get_id()));
		return m_CommandPools.at(std::this_thread::get_id());
	}

	Ref<DirectXCommandPool> DirectXDevice::GetOrCreateThreadLocalCommandPool()
	{
		if (m_CommandPools.contains(std::this_thread::get_id()))
			return m_CommandPools.at(std::this_thread::get_id());

		Ref<DirectXCommandPool> commandPool = Ref<DirectXCommandPool>::Create();
		m_CommandPools[std::this_thread::get_id()] = commandPool;
		return commandPool;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Command Pool /////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DirectXCommandPool::DirectXCommandPool()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		DirectXAPI::CreateDeferredContext(dxDevice, m_Context);
	}

	DirectXCommandPool::~DirectXCommandPool()
	{
		DX_DESTROYED(m_Context->Release(), DX_DESTROYED_LOG_ERROR);
		m_Context = nullptr;
	}

	ID3D11DeviceContext* DirectXCommandPool::AllocateCommandBuffer()
	{
		return m_Context;
	}

	void DirectXCommandPool::FlushCommandBuffer(ID3D11DeviceContext* commandBuffer)
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		ID3D11CommandList* commandList;
		commandBuffer->FinishCommandList(false, &commandList);

		device->Lock();
		auto queue = device->GetQueue();
		queue->ExecuteCommandList(commandList, false);
		device->Unlock();

		commandList->Release();
		commandBuffer->ClearState();
	}

}
