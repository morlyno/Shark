#pragma once
#include <Shark.h>
#include <imgui.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer(const std::string& name = std::string{});
	void OnAttach() override;

	void OnUpdate(Shark::TimeStep ts) override;
	void OnImGuiRender() override;

	void OnEvent(Shark::Event& e) override;

private:
	Shark::OrtographicCameraController m_CameraController;

	Shark::Ref<Shark::Shaders> m_Shaders;
	Shark::Ref<Shark::VertexBuffer> m_VertexBufferTriangle;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferTriangle;

	Shark::Ref<Shark::VertexBuffer> m_VertexBufferSquare;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferSquare;

	DirectX::XMMATRIX m_SquareTranslation = DirectX::XMMatrixIdentity();
	float rotation = 0;
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scal = { 1.0f, 1.0f, 1.0f };
};
