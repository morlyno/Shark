#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Layer/Layer.h"
#include "Shark/Render/Image.h"

#include <imgui.h>

#undef CreateWindow

namespace Shark {

	class ImGuiRenderer;

	class ImGuiLayer : public Layer
	{
	public:
		static ImGuiLayer* Create() { return sknew ImGuiLayer(); }

		void OnAttach();
		void OnDetach();

		void OnEvent(Event& event);

		void Begin();
		void End();

		void SetMainViewportID(ImGuiID mainViewportID) { m_MainViewportID = m_MainViewportID; }
		ImGuiID GetMainViewportID() const { return m_MainViewportID; }

		TimeStep GetGPUTime() const { return 0.0f; }

		bool BlocksMouseEvents() const { return m_BlockEvents && ImGui::GetIO().WantCaptureMouse; }
		bool BlocksKeyboardEvents() const { return m_BlockEvents && ImGui::GetIO().WantCaptureKeyboard; }
		void BlockEvents(bool block) { m_BlockEvents = block; }

		void AddViewable(Ref<ViewableResource> viewable) { m_Viewables.push_back(viewable); }

	private:
		ImGuiLayer();

		friend class Callbacks;
		void CreateWindow(ImGuiViewport* viewport);
		void DestroyWindow(ImGuiViewport* viewport);
		void SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
		void RenderWindow(ImGuiViewport* viewport, void*);
		void SwapBuffers(ImGuiViewport* viewport, void*);
	private:
		Scope<ImGuiRenderer> m_Renderer;

		ImGuiID m_MainViewportID;
		bool m_BlockEvents = false;

		std::vector<Ref<ViewableResource>> m_Viewables;
	};

}