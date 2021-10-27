#pragma once

#include "Shark/ImGui/ImGuiLayer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXImGuiLayer : ImGuiLayer
	{
	public:
		DirectXImGuiLayer();
		~DirectXImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(Event& event) override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void BlockEvents(bool block) override { m_BlockEvents = block; }
		virtual void SubmitBlendCallback(bool blend) override;

		virtual void SetDarkStyle() override;
	private:
		bool m_BlockEvents = false;

		Ref<DirectXRenderCommandBuffer> m_CommandBuffer;

		ID3D11BlendState* m_BlendState = nullptr;
	};

}
