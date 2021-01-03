#pragma once
#include <Shark.h>
#include <imgui.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer(const std::string& name = std::string{});
	void OnAttach() override;

	void OnUpdate(Shark::TimeStep ts) override;
	void OnRender() override;
	void OnImGuiRender() override;

	void OnEvent(Shark::Event& e) override;
	bool OnWindowResize(Shark::WindowResizeEvent e);

private:
	Shark::OrtographicCamera m_Camera;

	Shark::Ref<Shark::Shaders> m_Shaders;
	Shark::Ref<Shark::VertexBuffer> m_VertexBufferTriangle;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferTriangle;

	Shark::Ref<Shark::VertexBuffer> m_VertexBufferSquare;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferSquare;
};
