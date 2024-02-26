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

	void DirectXRenderCommandBuffer::Execute(Ref<GPUPipelineQuery> query)
	{
		if (m_Type == CommandBufferType::Immediate)
			return;

		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, dxQuery = query.As<DirectXGPUPipelineQuery>()]()
		{
			auto context = DirectXRenderer::Get()->GetContext();
			dxQuery->Begin(context);
			context->ExecuteCommandList(instance->m_CommandList, false);
			dxQuery->End(context);
		});
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

	void DirectXRenderCommandBuffer::BeginQuery(Ref<GPUTimer> query)
	{
		Renderer::Submit([query, context = m_Context]()
		{
			query.As<DirectXGPUTimer>()->RT_StartQuery(context);
		});
	}

	void DirectXRenderCommandBuffer::BeginQuery(Ref<GPUPipelineQuery> query)
	{
		Renderer::Submit([query, context = m_Context]()
		{
			query.As<DirectXGPUPipelineQuery>()->Begin(context);
		});
	}

	void DirectXRenderCommandBuffer::EndQuery(Ref<GPUTimer> query)
	{
		Renderer::Submit([query, context = m_Context]()
		{
			query.As<DirectXGPUTimer>()->RT_EndQuery(context);
		});
	}

	void DirectXRenderCommandBuffer::EndQuery(Ref<GPUPipelineQuery> query)
	{
		Renderer::Submit([query, context = m_Context]()
		{
			query.As<DirectXGPUPipelineQuery>()->End(context);
		});
	}

	void DirectXRenderCommandBuffer::RT_BeginQuery(Ref<DirectXGPUTimer> query)
	{
		query->RT_StartQuery(m_Context);
	}

	void DirectXRenderCommandBuffer::RT_BeginQuery(Ref<DirectXGPUPipelineQuery> query)
	{
		query->Begin(m_Context);
	}

	void DirectXRenderCommandBuffer::RT_EndQuery(Ref<DirectXGPUTimer> query)
	{
		query->RT_EndQuery(m_Context);
	}

	void DirectXRenderCommandBuffer::RT_EndQuery(Ref<DirectXGPUPipelineQuery> query)
	{
		query->End(m_Context);
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
