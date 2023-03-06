#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/Windows/WindowsUtils.h"

namespace Shark {


	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		Renderer::Submit([instance, renderer]()
		{
			Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
			SK_DX11_CALL(renderer->GetDevice()->CreateDeferredContext(0, &instance->m_DeferredContext));
		});
		renderer->AddCommandBuffer(this);
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		Release();

		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		renderer->RemoveCommandBuffer(this);
	}

	void DirectXRenderCommandBuffer::Release()
	{
		Renderer::SubmitResourceFree([context = m_DeferredContext, commandList = m_CommandList]()
		{
			if (commandList)
				commandList->Release();
			if (context)
				context->Release();
		});

		m_CommandList = nullptr;
		m_DeferredContext = nullptr;
	}

	void DirectXRenderCommandBuffer::Begin()
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Begin();
		});
	}

	void DirectXRenderCommandBuffer::End()
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_End();
		});
	}

	void DirectXRenderCommandBuffer::Execute()
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Execute();
		});
	}

	void DirectXRenderCommandBuffer::BeginTimeQuery(Ref<GPUTimer> counter)
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, counter]()
		{
			instance->RT_BeginTimeQuery(counter);
		});
	}

	void DirectXRenderCommandBuffer::EndTimeQuery(Ref<GPUTimer> counter)
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, counter]()
		{
			instance->RT_EndTimeQuery(counter);
		});
	}

	void DirectXRenderCommandBuffer::RT_ClearState()
	{
		m_DeferredContext->Flush();
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

	void DirectXRenderCommandBuffer::RT_Begin()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		m_Active = true;
	}

	void DirectXRenderCommandBuffer::RT_End()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_CommandList)
			m_CommandList->Release();

		SK_DX11_CALL(m_DeferredContext->FinishCommandList(FALSE, &m_CommandList));
		m_Active = false;
	}

	void DirectXRenderCommandBuffer::RT_Execute()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto context = DirectXRenderer::GetContext();
		context->ExecuteCommandList(m_CommandList, FALSE);
	}

	void DirectXRenderCommandBuffer::RT_BeginTimeQuery(Ref<GPUTimer> timer)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		Ref<DirectXGPUTimer> dxTimer = timer.As<DirectXGPUTimer>();
		dxTimer->RT_StartQuery(m_DeferredContext);
	}

	void DirectXRenderCommandBuffer::RT_EndTimeQuery(Ref<GPUTimer> timer)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		Ref<DirectXGPUTimer> dxTimer = timer.As<DirectXGPUTimer>();
		dxTimer->RT_EndQuery(m_DeferredContext);
	}

}
