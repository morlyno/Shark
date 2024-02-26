#pragma once

#include "Shark/Render/RenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"
#include <d3d11.h>

namespace Shark {

	enum class CommandBufferType
	{
		Immediate,
		Deferred
	};

	class DirectXRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		DirectXRenderCommandBuffer();
		DirectXRenderCommandBuffer(CommandBufferType type, ID3D11DeviceContext* context);
		virtual ~DirectXRenderCommandBuffer();

		virtual void Release() override;

		ID3D11DeviceContext* GetContext() const { return m_Context; }

		virtual void Begin() override;
		virtual void End() override;
		virtual void Execute() override;
		virtual void Execute(Ref<GPUPipelineQuery> query) override;

		void RT_Begin();
		void RT_End();
		void RT_Execute();

		void RT_ClearState();

		virtual void BeginQuery(Ref<GPUTimer> query) override;
		virtual void BeginQuery(Ref<GPUPipelineQuery> query) override;
		virtual void EndQuery(Ref<GPUTimer> query) override;
		virtual void EndQuery(Ref<GPUPipelineQuery> query) override;

		void RT_BeginQuery(Ref<DirectXGPUTimer> query);
		void RT_BeginQuery(Ref<DirectXGPUPipelineQuery> query);
		void RT_EndQuery(Ref<DirectXGPUTimer> query);
		void RT_EndQuery(Ref<DirectXGPUPipelineQuery> query);

	public:
		void CreateDeferredContext();
		void RegisterDeferred();
		void UnregisterDeferred();

	private:
		CommandBufferType m_Type = CommandBufferType::Deferred;

		ID3D11DeviceContext* m_Context = nullptr;
		ID3D11CommandList* m_CommandList = nullptr;
	};

}
