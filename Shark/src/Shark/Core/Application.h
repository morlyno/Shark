#pragma once

#include "Core.h"
#include "Window.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Layer/LayerStack.h"
#include "Shark/ImGui/ImGuiLayer.h"

#include "Shark/Render/Shaders.h"
#include "Shark/Render/Buffers.h"

namespace Shark {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		int Run();

		void OnEvent( Event& e );

		void PushLayer( Layer* layer ) { m_LayerStack.PushLayer( layer ); }
		void PopLayer( Layer* layer ) { m_LayerStack.PopLayer( layer ); }
		void PushOverlay( Layer* layer ) { m_LayerStack.PushOverlay( layer ); }
		void PopOverlay( Layer* layer ) { m_LayerStack.PopOverlay( layer ); }

		static inline Application* Get() { return s_inst; }
		inline Window* GetWindow() { return m_Window.get(); }
	private:
		bool OnWindowClose( WindowCloseEvent& e );
		bool OnWindowResize( WindowResizeEvent& e );

		void InitDrawTrinagle();
		void DrawTriangle();

		std::unique_ptr<Shaders> m_Shaders;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
		//Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
		//Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
		//Microsoft::WRL::ComPtr<ID3D11Buffer> PSConstantBuffer;
	private:
		static Application* s_inst;

		bool m_Running = true;
		int m_ExitCode = -1;
		float m_LastFrameTime = 0.0f;

		std::unique_ptr<Window> m_Window;
		// Owned by LayerStack
		ImGuiLayer* m_pImGuiLayer;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();

}

