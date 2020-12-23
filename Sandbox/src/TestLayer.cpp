#include "TestLayer.h"

TestLayer::TestLayer(const std::string& name)
	:
	Layer("TestLayer" + name),
	m_Camera(-(16.0f / 9.0f), 16.0f / 9.0f, -1.0, 1.0f)
{
}

void TestLayer::OnAttach()
{
	std::string vs = R"(
			cbuffer cbuff : register(b0)
			{
				float4x4 ViewProjection;
			}

			struct VSOUT
			{
				float4 color : Color;
				float4 pos : SV_POSITION;
			};

			VSOUT main( float3 pos : Position,float4 color : Color )
			{
				VSOUT vsout;
				vsout.pos = mul(ViewProjection, float4(pos, 1.0f));
				//vsout.pos = float4(pos, 1.0f);
				vsout.color = color;
				return vsout;
			}
		)";

	std::string ps = R"(
			float4 main( float4 color : Color ) : SV_TARGET
			{
				return color;
			}
		)";

	Shark::VertexLayout layout =
	{
		{ Shark::VertexElement::Float3,"Position" },
		{ Shark::VertexElement::Float4,"Color" }
	};

	m_Shaders = Shark::Shaders::Create(layout, vs, ps);


	float Trianglevertices[3 * 7] =
	{
		-0.5f,-0.5f, 0.0f, 0.8f, 0.2f, 0.1f, 1.0f,
		 0.0f, 0.5f, 0.0f, 0.2f, 0.1f, 0.8f, 1.0f,
		 0.5f,-0.5f, 0.0f, 0.1f, 0.8f, 0.2f, 1.0f,
	};
	uint32_t Triangleindices[] = { 0,1,2 };

	m_VertexBufferTriangle = Shark::VertexBuffer::Create(layout, Trianglevertices, 3);
	m_IndexBufferTriangle = Shark::IndexBuffer::Create(Triangleindices, (uint32_t)std::size(Triangleindices));



	float Squarevertices[4 * 7] =
	{
		-1.0f,-1.0f, 0.0f, 0.8f, 0.2f, 0.1f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f,
		 1.0f, 1.0f, 0.0f, 0.2f, 0.1f, 0.8f, 1.0f,
		 1.0f,-1.0f, 0.0f, 0.1f, 0.8f, 0.2f, 1.0f,
	};
	uint32_t Squareindices[] = { 0,1,2, 2,3,0 };

	m_VertexBufferSquare = Shark::VertexBuffer::Create(layout, Squarevertices, 4);
	m_IndexBufferSquare = Shark::IndexBuffer::Create(Squareindices, (uint32_t)std::size(Squareindices));
}

void TestLayer::OnRender()
{
	Shark::Renderer::BeginScean(m_Camera);
	Shark::Renderer::Submit(m_VertexBufferSquare, m_IndexBufferSquare, m_Shaders);
	Shark::Renderer::Submit(m_VertexBufferTriangle, m_IndexBufferTriangle, m_Shaders);
	Shark::Renderer::EndScean();
}

void TestLayer::OnImGuiRender()
{
	ImGui::ShowDemoWindow();
}

void TestLayer::OnEvent(Shark::Event& e)
{
	Shark::EventDispacher dispacher(e);
	dispacher.DispachEvent<Shark::WindowResizeEvent>(SK_BIND_EVENT_FN(TestLayer::OnWindowResize));
}

bool TestLayer::OnWindowResize(Shark::WindowResizeEvent e)
{
	float ratio = (float)e.GetWidth() / (float)e.GetHeight();
	m_Camera.SetProjection(-ratio, ratio, -1.0, 1.0f);
	return false;
};