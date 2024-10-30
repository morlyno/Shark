#include "skpch.h"
#include "DirectXContext.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/Windows/WindowsUtils.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include <dxgi1_3.h>

namespace Shark {

	namespace utils {

		static std::string GetGPUDescription(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad = {};
			DX11_VERIFY(adapter->GetDesc(&ad));
			return String::ToNarrow(std::wstring_view(ad.Description));
		}

	}

	DirectXContext::DirectXContext()
	{
		m_Device = Ref<DirectXDevice>::Create();
		CreateInfoQueue();
	}

	DirectXContext::~DirectXContext()
	{
		m_InfoQueue->Release();
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

		FlushMessages();

		static bool BreakOnError = true;
		if (BreakOnError)
		{
			SK_DEBUG_BREAK();
		}

		//SK_CORE_ERROR_TAG("Renderer", WindowsUtils::TranslateHResult(hr));
	}

	void DirectXContext::ReportLiveObjects()
	{
		IDXGIDebug1* debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
			FlushMessages();
			debug->Release();
		}
	}

	void DirectXContext::FlushMessages()
	{
		FlushDXMessages();

		const auto& producer = DXGI_DEBUG_ALL;

		Buffer messageBuffer;
		uint64_t count = m_InfoQueue->GetNumStoredMessages(producer);
		for (uint64_t i = 0; i < count; i++)
		{
			uint64_t messageLength;
			DX11_VERIFY(m_InfoQueue->GetMessage(producer, i, nullptr, &messageLength));

			messageBuffer.Resize(messageLength, false);
			auto message = messageBuffer.As<DXGI_INFO_QUEUE_MESSAGE>();

			DX11_VERIFY(m_InfoQueue->GetMessage(producer, i, message, &messageLength));

			switch (message->Severity)
			{
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE: SK_CORE_TRACE_TAG("Renderer", "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO: SK_CORE_INFO_TAG("Renderer", "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: SK_CORE_WARN_TAG("Renderer", "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR: SK_CORE_ERROR_TAG("Renderer", "{}", message->pDescription); break;
				case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: SK_CORE_CRITICAL_TAG("Renderer", "{}", message->pDescription); break;
				default:
				{
					SK_DEBUG_BREAK_CONDITIONAL(s_BreakShouldNotHappen);
					SK_CORE_WARN_TAG("Renderer", "Unkown Severity! {}", message->pDescription);
					break;
				}
			}
		}

		m_InfoQueue->ClearStoredMessages(producer);
		messageBuffer.Release();
	}

	void DirectXContext::FlushDXMessages()
	{
		auto dxDevice = m_Device->GetDirectXDevice();

		ID3D11InfoQueue* infoQueue;
		HRESULT hr = dxDevice->QueryInterface(&infoQueue);
		DX11_VERIFY(hr);
		if (SUCCEEDED(hr))
		{
			Buffer messageBuffer;
			uint64_t count = infoQueue->GetNumStoredMessages();
			for (uint64_t i = 0; i < count; i++)
			{
				uint64_t messageLength;
				if (HRESULT hr = infoQueue->GetMessage(i, nullptr, &messageLength); FAILED(hr))
				{
					SK_CORE_ERROR_TAG("Renderer", "Failed to receive message from InfoQueue");
					continue;
				}

				messageBuffer.Resize(messageLength, false);
				auto message = messageBuffer.As<D3D11_MESSAGE>();

				DX11_VERIFY(infoQueue->GetMessage(i, message, &messageLength));

				m_InfoQueue->AddMessage(DXGI_DEBUG_D3D11,
										(DXGI_INFO_QUEUE_MESSAGE_CATEGORY)message->Category,
										(DXGI_INFO_QUEUE_MESSAGE_SEVERITY)message->Severity,
										(DXGI_INFO_QUEUE_MESSAGE_ID)message->ID,
										message->pDescription
				);

			}

			infoQueue->ClearStoredMessages();
			infoQueue->Release();
			messageBuffer.Release();
		}
	}

	Ref<DirectXDevice> DirectXContext::GetCurrentDevice()
	{
		return Renderer::GetRendererContext().As<DirectXContext>()->m_Device;
	}

	Ref<DirectXContext> DirectXContext::Get()
	{
		return Renderer::GetRendererContext().As<DirectXContext>();
	}

	void DirectXContext::CreateInfoQueue()
	{
		DX11_VERIFY(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_InfoQueue)));
		m_InfoQueue->PushEmptyRetrievalFilter(DXGI_DEBUG_ALL);
		m_InfoQueue->PushEmptyStorageFilter(DXGI_DEBUG_ALL);

		bool enableBreakOnSeverity = false;
		if (enableBreakOnSeverity)
		{
			m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			m_InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}

#if 1
		auto dxDevice = m_Device->GetDirectXDevice();
		ID3D11InfoQueue* infoQueue = nullptr;
		dxDevice->QueryInterface(&infoQueue);

		if (enableBreakOnSeverity)
		{
			DX11_VERIFY(infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true));
			DX11_VERIFY(infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true));
		}

		D3D11_MESSAGE_ID deniedMessages[] = { D3D11_MESSAGE_ID_UNKNOWN };
		D3D11_MESSAGE_SEVERITY deniedSeverities[] = { D3D11_MESSAGE_SEVERITY_MESSAGE, D3D11_MESSAGE_SEVERITY_INFO };

		D3D11_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = (UINT)std::size(deniedMessages);
		filter.DenyList.pIDList = deniedMessages;
		filter.DenyList.NumSeverities = (UINT)std::size(deniedSeverities);
		filter.DenyList.pSeverityList = deniedSeverities;
		DX11_VERIFY(infoQueue->PushRetrievalFilter(&filter));
		DX11_VERIFY(infoQueue->PushStorageFilter(&filter));

		infoQueue->Release();
#endif
	}



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

		ID3D11Query* query = nullptr;
		DirectXAPI::CreateQuery(m_Device, D3D11_QUERY_TIMESTAMP_DISJOINT, query);
		m_Queue->Begin(query);
		m_Queue->End(query);
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
		HRESULT result = m_Queue->GetData(query, &data, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
		DX11_VERIFY(result);
		m_Limits.TimestampPeriod = data.Frequency;
	}

	DirectXDevice::~DirectXDevice()
	{
		m_Queue->Release();
		m_Device->Release();
		m_Factory->Release();
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



	DirectXCommandPool::DirectXCommandPool()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		for (uint32_t i = 0; i < 3; i++)
		{
			ID3D11DeviceContext* context;
			DirectXAPI::CreateDeferredContext(dxDevice, context);

			CommandBuffer cmdBuffer;
			cmdBuffer.Context = context;
			cmdBuffer.InUse = false;
			m_Pool.push_front(cmdBuffer);
		}
	}

	DirectXCommandPool::~DirectXCommandPool()
	{
		for (const auto& cmdBuffer : m_Pool)
		{
			SK_CORE_ASSERT(cmdBuffer.InUse == false);
			cmdBuffer.Context->Release();
		}
	}

	ID3D11DeviceContext* DirectXCommandPool::AllocateCommandBuffer()
	{
		SK_PROFILE_FUNCTION();
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		CommandBuffer* cmdBuffer = GetNextAvailableCommandBuffer();
		return cmdBuffer->Context;
	}

	void DirectXCommandPool::FlushCommandBuffer(ID3D11DeviceContext* commandBuffer)
	{
		SK_PROFILE_FUNCTION();
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
		ReleaseCommandBuffer(commandBuffer);
	}

	DirectXCommandPool::CommandBuffer* DirectXCommandPool::GetNextAvailableCommandBuffer()
	{
		if (m_CommandBuffersInUse < m_Pool.size())
		{
			auto i = m_Pool.rend();
			for (i = m_Pool.rbegin(); i != m_Pool.rend(); i++)
			{
				if (!i->InUse)
				{
					m_CommandBuffersInUse++;
					return &*i;
				}
			}

			SK_CORE_VERIFY(false);
		}

		SK_CORE_WARN_TAG("Renderer", "Creating CommandBuffer (Pool size: {})", m_Pool.size());
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		ID3D11DeviceContext* context;
		DirectXAPI::CreateDeferredContext(dxDevice, context);

		CommandBuffer cmdBuffer;
		cmdBuffer.Context = context;
		cmdBuffer.InUse = true;
		m_Pool.push_front(cmdBuffer);
		m_CommandBuffersInUse++;

		return &m_Pool.front();
	}

	void DirectXCommandPool::ReleaseCommandBuffer(ID3D11DeviceContext* commandBuffer)
	{
		auto i = m_Pool.begin();
		for (; i != m_Pool.end(); i++)
		{
			if (i->Context == commandBuffer)
			{
				i->InUse = false;
				m_CommandBuffersInUse--;
				return;
			}
		}

		SK_CORE_VERIFY(false);
	}

}
