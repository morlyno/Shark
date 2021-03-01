#pragma once
#include <Shark.h>
#include <Shark/Render/OrtographicCameraController.h>
#include <imgui.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer(const std::string& name);
	void OnAttach() override;

	void OnUpdate(Shark::TimeStep ts) override;
	void OnImGuiRender() override;

	void OnEvent(Shark::Event& e) override;

private:
	Shark::OrtographicCameraController m_CameraController;

	Shark::Ref<Shark::Shaders> m_Shaders;
	Shark::Ref<Shark::Shaders> m_TextureShaders;
	Shark::Ref<Shark::VertexBuffer> m_VertexBufferTriangle;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferTriangle;

	Shark::Ref<Shark::VertexBuffer> m_VertexBufferSquare;
	Shark::Ref<Shark::VertexBuffer> m_VertexBufferSquareTexture;
	Shark::Ref<Shark::IndexBuffer> m_IndexBufferSquare;

	Shark::Ref<Shark::Texture2D> m_Texture;
	Shark::Ref<Shark::Texture2D> m_TextureS;

	DirectX::XMMATRIX m_SquareTranslation = DirectX::XMMatrixIdentity();
	float rotation = 0;
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scal = { 1.0f, 1.0f, 1.0f };
};
