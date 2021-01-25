#include "TestLayer.h"

TestLayer::TestLayer(const std::string& name)
	:
	Layer(name),
	m_CameraController(1280.0f / 720.0f, true)
{
}

void TestLayer::OnAttach()
{
	m_Shaders = Shark::Shaders::Create("assets/Shaders/TestShader.hlsl");
	m_TextureShaders = Shark::Shaders::Create("assets/Shaders/TextureShader.hlsl");
	m_Texture = Shark::Texture2D::Create("assets/Textures/Checkerboard.png");
	m_TextureS = Shark::Texture2D::Create("assets/Textures/S.png");

	float Trianglevertices[3 * 7] =
	{
		-0.5f,-0.5f, 0.0f, 0.8f, 0.2f, 0.1f, 0.5f,
		 0.0f, 0.5f, 0.0f, 0.2f, 0.1f, 0.8f, 0.5f,
		 0.5f,-0.5f, 0.0f, 0.1f, 0.8f, 0.2f, 0.5f,
	};
	uint32_t Triangleindices[] = { 0,1,2 };

	m_VertexBufferTriangle = Shark::VertexBuffer::Create(m_Shaders->GetVertexLayout(), Trianglevertices, sizeof(Trianglevertices));
	m_IndexBufferTriangle = Shark::IndexBuffer::Create(Triangleindices, (uint32_t)std::size(Triangleindices));


	float TextureSquarevertices[4 * 5] =
	{
		-1.0f,-1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		 1.0f,-1.0f, 0.0f, 1.0f, 1.0f
	};
	m_VertexBufferSquareTexture = Shark::VertexBuffer::Create(m_TextureShaders->GetVertexLayout(), TextureSquarevertices, sizeof(TextureSquarevertices));

	float Squarevertices[4 * 7] =
	{
		-1.0f,-1.0f, 0.0f, 0.8f, 0.2f, 0.1f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
		 1.0f, 1.0f, 0.0f, 0.2f, 0.1f, 0.8f, 1.0f,
		 1.0f,-1.0f, 0.0f, 0.1f, 0.8f, 0.2f, 1.0f,
	};
	uint32_t Squareindices[] = { 0,1,2, 2,3,0 };

	m_VertexBufferSquare = Shark::VertexBuffer::Create(m_Shaders->GetVertexLayout(), Squarevertices, sizeof(Squarevertices));
	m_IndexBufferSquare = Shark::IndexBuffer::Create(Squareindices, (uint32_t)std::size(Squareindices));
}

void TestLayer::OnUpdate(Shark::TimeStep ts)
{
	m_CameraController.OnUpdate(ts);

	Shark::Renderer::BeginScean(m_CameraController.GetCamera());
	DirectX::XMMATRIX translation = DirectX::XMMatrixRotationZ(0.5f) * DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f) * DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	Shark::Renderer::Submit(m_VertexBufferSquare, m_IndexBufferSquare, m_Shaders, m_SquareTranslation);
	Shark::Renderer::Submit(m_VertexBufferTriangle, m_IndexBufferTriangle, m_Shaders, translation);
	//Shark::Renderer::Submit(m_VertexBufferSquareTexture, m_IndexBufferSquare, m_TextureShaders, m_Texture, DirectX::XMMatrixIdentity());
	Shark::Renderer::Submit(m_VertexBufferSquareTexture, m_IndexBufferSquare, m_TextureShaders, m_TextureS, DirectX::XMMatrixIdentity());
	Shark::Renderer::EndScean();
}

void TestLayer::OnImGuiRender()
{
	bool changed = false;
	if (ImGui::Begin("Squera Translation"))
	{
		changed |= ImGui::SliderAngle("Rotation", &rotation);
		changed |= ImGui::DragFloat3("Position", &pos.x);
		changed |= ImGui::DragFloat3("Scaling", &scal.x);
	}
	ImGui::End();

	if (changed)
	{
		m_SquareTranslation = DirectX::XMMatrixScaling(scal.x, scal.y, scal.z) * DirectX::XMMatrixRotationZ(rotation) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	}
}

void TestLayer::OnEvent(Shark::Event& e)
{
	m_CameraController.OnEvent(e);
}
