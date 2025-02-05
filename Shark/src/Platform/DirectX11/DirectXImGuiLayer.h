#pragma once

#include "Shark/UI/ImGui/ImGuiLayer.h"
#include "Platform/DirectX11/DirectXImage.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

#include <imgui.h>

#include <d3d11.h>

namespace Shark {

	class DirectXImGuiLayer : public ImGuiLayer
	{
	public:
		DirectXImGuiLayer();
		~DirectXImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnBeginUI() override;
		virtual void OnEndUI() override;

		virtual bool InFrame() const override { return m_InFrame; }
		virtual void SetMainViewportID(ImGuiID mainViewportID) override { m_MainViewportID = mainViewportID; }
		virtual ImGuiID GetMainViewportID() const override { return m_MainViewportID; }

		virtual TimeStep GetGPUTime() const override { return m_GPUTime; }

		virtual bool BlocksMouseEvents() const override { return m_BlockEvents && ImGui::GetIO().WantCaptureMouse; }
		virtual bool BlocksKeyboardEvents() const override { return m_BlockEvents && ImGui::GetIO().WantCaptureKeyboard; }
		virtual void BlockEvents(bool block) override { m_BlockEvents = block; }

		virtual void AddImage(Ref<Image2D> image) override;
		virtual void AddImage(Ref<ImageView> view) override;
		virtual void BindFontSampler() override;

		Ref<DirectXRenderCommandBuffer> GetDirectXCommandBuffer() const { return m_CommandBuffer; }

	private:
		bool m_BlockEvents = false;
		bool m_InFrame = false;
		ImGuiID m_MainViewportID = 0;

		Ref<DirectXRenderCommandBuffer> m_CommandBuffer;
		ID3D11SamplerState* m_ImGuiFontSampler = nullptr;

		std::unordered_map<ImTextureID, Ref<DirectXImage2D>> m_ImageMap;

		uint32_t m_TimestampQuery;
		TimeStep m_GPUTime;

		friend class DirectXRenderer;
		friend void BindSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);
	};

}
