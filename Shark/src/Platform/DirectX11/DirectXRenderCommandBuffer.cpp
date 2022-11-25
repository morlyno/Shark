#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/Windows/WindowsUtils.h"

namespace Shark {


	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		SK_DX11_CALL(renderer->GetDevice()->CreateDeferredContext(0, &m_DeferredContext));
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

		SK_DX11_CALL(m_DeferredContext->FinishCommandList(FALSE, &m_CommandList));
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
		SK_DX11_CALL(m_DeferredContext->FinishCommandList(false, &dummyList));
		//SK_CORE_ASSERT(SUCCEEDED(hr), WindowsUtils::TranslateHResult(hr));
		dummyList->Release();

		if (m_CommandList)
		{
			m_CommandList->Release();
			m_CommandList = nullptr;
		}
	}

}
