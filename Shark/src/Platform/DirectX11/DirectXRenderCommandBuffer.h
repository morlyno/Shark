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

		void RT_Begin();
		void RT_End();
		void RT_Execute();

		virtual void BeginTimeQuery(Ref<GPUTimer> timer) override;
		virtual void EndTimeQuery(Ref<GPUTimer> timer) override;

		void RT_BeginTimeQuery(Ref<DirectXGPUTimer> timer);
		void RT_EndTimeQuery(Ref<DirectXGPUTimer> timer);

		void RT_ClearState();

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
