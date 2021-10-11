#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {
	
	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		auto dev = DirectXRendererAPI::GetDevice();
		SK_CHECK(dev->CreateDeferredContext(0, &m_DeferredContext));
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		if (m_CommandList)
			m_CommandList->Release();
		if (m_DeferredContext)
			m_DeferredContext->Release();
	}

	void DirectXRenderCommandBuffer::Begin()
	{
		m_DeferredContext->ClearState();
		
		auto api = DirectXRendererAPI::Get();
		api->SetActiveContext(m_DeferredContext);
	}

	void DirectXRenderCommandBuffer::End()
	{
		auto api = DirectXRendererAPI::Get();
		api->SetActiveContext(api->GetImmediateContext());
		
		if (m_CommandList)
			m_CommandList->Release();

		SK_CHECK(m_DeferredContext->FinishCommandList(FALSE, &m_CommandList));
	}

	void DirectXRenderCommandBuffer::Execute()
	{
		auto ctx = DirectXRendererAPI::GetImmediateContext();
		ctx->ExecuteCommandList(m_CommandList, FALSE);
	}

}
