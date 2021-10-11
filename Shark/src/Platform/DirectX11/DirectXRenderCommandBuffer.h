#pragma once

#include "Shark/Render/RenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		DirectXRenderCommandBuffer();
		virtual ~DirectXRenderCommandBuffer();

		ID3D11DeviceContext* GetContext() const { return m_DeferredContext; }

		virtual void Begin() override;
		virtual void End() override;
		virtual void Execute() override;

	private:
		ID3D11DeviceContext* m_DeferredContext = nullptr;
		ID3D11CommandList* m_CommandList = nullptr;
	};

}
