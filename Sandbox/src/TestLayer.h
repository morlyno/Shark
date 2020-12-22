#pragma once
#include <Shark.h>
#include <imgui.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer( const std::string& name = std::string{} );
	void OnAttach() override;
	void OnRender() override;
	void OnImGuiRender() override;

private:
	std::unique_ptr<Shark::VertexBuffer> m_VertexBuffer;
	std::unique_ptr<Shark::IndexBuffer> m_IndexBuffer;
	std::shared_ptr<Shark::Shaders> m_Shaders;
};
