#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/Windows/WindowsUtils.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) " 0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {


	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		SK_CHECK(renderer->GetDevice()->CreateDeferredContext(0, &m_DeferredContext));
		renderer->AddCommandBuffer(this);
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		if (m_CommandList)
			m_CommandList->Release();
		if (m_DeferredContext)
			m_DeferredContext->Release();

		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		renderer->RemoveCommandBuffer(this);
	}

	void DirectXRenderCommandBuffer::Begin()
	{
		m_Active = true;
		if (m_CommandList)
		{
			m_CommandList->Release();
			m_CommandList = nullptr;
		}
	}

	void DirectXRenderCommandBuffer::End()
	{
		if (m_CommandList)
			m_CommandList->Release();

		SK_CHECK(m_DeferredContext->FinishCommandList(FALSE, &m_CommandList));
		m_Active = false;
	}

	void DirectXRenderCommandBuffer::Execute()
	{
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

	void DirectXRenderCommandBuffer::ClearState()
	{
		m_DeferredContext->ClearState();
		ID3D11CommandList* dummyList;
		HRESULT hr = m_DeferredContext->FinishCommandList(false, &dummyList);
		SK_CORE_ASSERT(SUCCEEDED(hr), WindowsUtils::TranslateErrorCode(hr));
		dummyList->Release();

		if (m_CommandList)
		{
			m_CommandList->Release();
			m_CommandList = nullptr;
		}
	}

}
