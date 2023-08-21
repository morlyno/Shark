#pragma once

#include "Shark/Render/RenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"
#include <d3d11.h>

namespace Shark {

	class DirectXRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		DirectXRenderCommandBuffer();
		virtual ~DirectXRenderCommandBuffer();

		virtual void Release() override;

		ID3D11DeviceContext* GetContext() const { return m_DeferredContext; }

		virtual void Begin() override;
		virtual void End() override;
		virtual void Execute() override;

		virtual void BeginTimeQuery(Ref<GPUTimer> counter) override;
		virtual void EndTimeQuery(Ref<GPUTimer> counter) override;

		void RT_ClearState();

	public:
		void RT_Begin();
		void RT_End();
		void RT_Execute();

		void RT_BeginTimeQuery(Ref<DirectXGPUTimer> timer);
		void RT_EndTimeQuery(Ref<DirectXGPUTimer> timer);

	private:
		ID3D11DeviceContext* m_DeferredContext = nullptr;
		ID3D11CommandList* m_CommandList = nullptr;

		bool m_Active = false;
	};

}
