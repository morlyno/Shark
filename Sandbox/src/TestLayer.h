#pragma once
#include <Shark.h>
#include <imgui.h>

#include <Shark/Render/OrtographicCamera.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer(const std::string& name = std::string{});
	void OnAttach() override;
	void OnRender() override;
	void OnImGuiRender() override;

	void OnEvent(Shark::Event& e) override;
	bool OnWindowResize(Shark::WindowResizeEvent e);

private:
	Shark::OrtographicCamera m_Camera;

	std::shared_ptr<Shark::Shaders> m_Shaders;
	std::unique_ptr<Shark::VertexBuffer> m_VertexBufferTriangle;
	std::unique_ptr<Shark::IndexBuffer> m_IndexBufferTriangle;

	std::unique_ptr<Shark::VertexBuffer> m_VertexBufferSquare;
	std::unique_ptr<Shark::IndexBuffer> m_IndexBufferSquare;
};
