#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/RendererContext.h"
#include <d3d11_1.h>
#include <dxgidebug.h>

namespace Shark {

#define DX11_VERIFY(call_or_hresult)\
{\
	HRESULT _hresult = (call_or_hresult);\
	if (FAILED(_hresult))\
	{\
		auto context = ::Shark::DirectXContext::Get();\
		context->HandleError(_hresult);\
	}\
}

	class DirectXDevice;
	class DirectXCommandPool;

	struct DeviceLimits
	{
		uint64_t TimestampPeriod = 0;
	};

	class DirectXContext : public RendererContext
	{
	public:
		DirectXContext();
		~DirectXContext();

		virtual void DestroyDevice() override;

		void HandleError(HRESULT hr);
		void FlushMessages();
		virtual void ReportLiveObjects() override;

		Ref<DirectXDevice> GetDevice() const { return m_Device; }
		static Ref<DirectXDevice> GetCurrentDevice();
		static Ref<DirectXContext> Get();
	private:
		void CreateInfoQueue();
		void FlushDXMessages();

	private:
		Ref<DirectXDevice> m_Device;
		IDXGIInfoQueue* m_InfoQueue = nullptr;
	};

	class DirectXDevice : public RefCount
	{
	public:
		DirectXDevice();
		~DirectXDevice();

		void MapMemory(ID3D11Resource* resource, uint32_t subresource, D3D11_MAP mapType, void*& outMemory);
		void MapMemory(ID3D11Resource* resource, uint32_t subresource, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE& outMapped);
		void UnmapMemory(ID3D11Resource* resource, uint32_t subresource);

		void UpdateSubresource(ID3D11Resource* destination, uint32_t destSubresource, const D3D11_BOX* destBox, const void* sourceData, uint32_t sourceRowPitch, uint32_t sourceDepthPitch);
		void CopySubresource(ID3D11Resource* destination, uint32_t destSubresource, uint32_t destX, uint32_t dextY, uint32_t destZ, ID3D11Resource* source, uint32_t srcSubresource, const D3D11_BOX* srcBox);

		ID3D11DeviceContext* AllocateCommandBuffer();
		void FlushCommandBuffer(ID3D11DeviceContext* commandBuffer);

		const DeviceLimits& GetLimits() const { return m_Limits; }

		ID3D11Device1* GetDirectXDevice() { return m_Device; }
		IDXGIFactory* GetDirectXFactory() { return m_Factory; }
		ID3D11DeviceContext* GetQueue() { return m_Queue; }

		void Lock() { m_SubmissionMutex.lock(); }
		void Unlock() { m_SubmissionMutex.unlock(); }

	private:
		Ref<DirectXCommandPool> GetThreadLocalCommandPool();
		Ref<DirectXCommandPool> GetOrCreateThreadLocalCommandPool();

	private:
		ID3D11Device1* m_Device = nullptr;
		IDXGIFactory* m_Factory = nullptr;

		std::mutex m_SubmissionMutex;
		ID3D11DeviceContext* m_Queue = nullptr;

		DeviceLimits m_Limits;

		std::map<std::thread::id, Ref<DirectXCommandPool>> m_CommandPools;
	};

	class DirectXCommandPool : public RefCount
	{
	public:
		DirectXCommandPool();
		~DirectXCommandPool();

		ID3D11DeviceContext* AllocateCommandBuffer();
		void FlushCommandBuffer(ID3D11DeviceContext* commandBuffer);

	private:
		ID3D11DeviceContext* m_Context = nullptr;
	};

}
