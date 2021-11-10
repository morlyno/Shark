#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) " 0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {


	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		auto dev = DirectXRenderer::GetDevice();
		SK_CHECK(dev->CreateDeferredContext(0, &m_DeferredContext));

		DirectXRenderer::AddRenderCommandBuffer(this);
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		DirectXRenderer::RemoveRenderCommandBuffer(this);

		if (m_CommandList)
			m_CommandList->Release();
		if (m_DeferredContext)
			m_DeferredContext->Release();
	}

	void DirectXRenderCommandBuffer::Begin()
	{
	}

	void DirectXRenderCommandBuffer::End()
	{
	}

	void DirectXRenderCommandBuffer::Execute()
	{
		if (m_CommandList)
			m_CommandList->Release();

		SK_CHECK(m_DeferredContext->FinishCommandList(FALSE, &m_CommandList));
		
		auto ctx = DirectXRenderer::GetContext();
		ctx->ExecuteCommandList(m_CommandList, FALSE);
	}

	void DirectXRenderCommandBuffer::BeginTimeQuery(Ref<GPUTimer> counter)
	{
		Ref<DirectXGPUTimer> dxCounter = counter.As<DirectXGPUTimer>();
		dxCounter->StartQuery(m_DeferredContext);
	}

	void DirectXRenderCommandBuffer::EndTimeQuery(Ref<GPUTimer> counter)
	{
		Ref<DirectXGPUTimer> dxCounter = counter.As<DirectXGPUTimer>();
		dxCounter->EndQuery(m_DeferredContext);
	}

	void DirectXRenderCommandBuffer::Flush()
	{
		Execute();
		m_CommandList->Release();
		m_CommandList = nullptr;
		m_DeferredContext->Flush();
		m_DeferredContext->ClearState();
	}

}
