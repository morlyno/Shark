#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

#include "Platform/Windows/WindowsUtils.h"

namespace Shark {

	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer()
	{
		CreateDeferredContext();
		RegisterDeferred();
	}

	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer(CommandBufferType type, ID3D11DeviceContext* context)
	{
		m_Type = type;
		m_Context = context;
		m_Context->AddRef();

		if (m_Type == CommandBufferType::Deferred)
		{
			RegisterDeferred();
		}
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		if (m_Type == CommandBufferType::Deferred)
		{
			UnregisterDeferred();
		}

		Release();
	}

	void DirectXRenderCommandBuffer::Release()
	{
		Renderer::SubmitResourceFree([context = m_Context, commandList = m_CommandList]()
		{
			if (commandList)
				commandList->Release();
			if (context)
				context->Release();
		});

		m_CommandList = nullptr;
		m_Context = nullptr;
	}

	void DirectXRenderCommandBuffer::Begin()
	{
	}

	void DirectXRenderCommandBuffer::End()
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			if (instance->m_CommandList)
				instance->m_CommandList->Release();

			SK_DX11_CALL(instance->m_Context->FinishCommandList(FALSE, &instance->m_CommandList));
		});
	}

	void DirectXRenderCommandBuffer::Execute()
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			auto context = DirectXRenderer::Get()->GetContext();
			context->ExecuteCommandList(instance->m_CommandList, FALSE);
		});
	}

	void DirectXRenderCommandBuffer::BeginTimeQuery(Ref<GPUTimer> timer)
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, dxTimer = timer.As<DirectXGPUTimer>()]()
		{
			dxTimer->RT_StartQuery(instance->m_Context);
		});
	}

	void DirectXRenderCommandBuffer::EndTimeQuery(Ref<GPUTimer> timer)
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, dxTimer = timer.As<DirectXGPUTimer>()]()
		{
			dxTimer->RT_EndQuery(instance->m_Context);
		});
	}

	void DirectXRenderCommandBuffer::RT_ClearState()
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		m_Context->Flush();
		m_Context->ClearState();
		ID3D11CommandList* dummyList;
		SK_DX11_CALL(m_Context->FinishCommandList(false, &dummyList));
		dummyList->Release();

		if (m_CommandList)
		{
			m_CommandList->Release();
			m_CommandList = nullptr;
		}
	}

	void DirectXRenderCommandBuffer::RT_Begin()
	{
	}

	void DirectXRenderCommandBuffer::RT_End()
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		if (m_CommandList)
			m_CommandList->Release();

		SK_DX11_CALL(m_Context->FinishCommandList(FALSE, &m_CommandList));
	}

	void DirectXRenderCommandBuffer::RT_Execute()
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		auto context = DirectXRenderer::GetContext();
		context->ExecuteCommandList(m_CommandList, FALSE);
	}

	void DirectXRenderCommandBuffer::RT_BeginTimeQuery(Ref<DirectXGPUTimer> timer)
	{
		timer->RT_StartQuery(m_Context);
	}

	void DirectXRenderCommandBuffer::RT_EndTimeQuery(Ref<DirectXGPUTimer> timer)
	{
		timer->RT_EndQuery(m_Context);
	}

	void DirectXRenderCommandBuffer::CreateDeferredContext()
	{
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();
		DirectXAPI::CreateDeferredContext(device, 0, m_Context);
	}

	void DirectXRenderCommandBuffer::RegisterDeferred()
	{
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		renderer->AddCommandBuffer(this);
	}

	void DirectXRenderCommandBuffer::UnregisterDeferred()
	{
		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		renderer->RemoveCommandBuffer(this);
	}

}
