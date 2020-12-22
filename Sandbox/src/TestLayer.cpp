#include "TestLayer.h"

TestLayer::TestLayer( const std::string& name )
	:
	Layer( "TestLayer" + name )
{
}

void TestLayer::OnAttach()
{
	const float clear_color[] = { 0.1f,0.1f,0.1f,1.0f };
	Shark::RendererCommand::SetClearColor( clear_color );

	std::string vs = R"(
			struct VSOUT
			{
				float4 color : Color;
				float4 pos : SV_POSITION;
			};

			VSOUT main( float3 pos : Position,float4 color : Color )
			{
				VSOUT vsout;
				vsout.pos = float4(pos, 1.0f);
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
		{ Shark::VertexElementType::Float3,"Position" },
		{ Shark::VertexElementType::Float4,"Color" }
	};

	m_Shaders = Shark::Shaders::Create( layout,vs,ps );

	float vertices[3 * 7] =
	{
		-0.5f,-0.5f, 0.0f, 0.8f, 0.2f, 0.1f, 1.0f,
		 0.0f, 0.5f, 0.0f, 0.2f, 0.1f, 0.8f, 1.0f,
		 0.5f,-0.5f, 0.0f, 0.1f, 0.8f, 0.2f, 1.0f,
	};
	uint32_t indices[] = { 0,1,2 };

	m_VertexBuffer = Shark::VertexBuffer::Create( layout,vertices,3 );

	m_IndexBuffer = Shark::IndexBuffer::Create( indices,3 );
}

void TestLayer::OnRender()
{
	Shark::Renderer::BeginScean();
	Shark::Renderer::Submit( m_VertexBuffer,m_IndexBuffer,m_Shaders );
	Shark::Renderer::EndScean();
}

void TestLayer::OnImGuiRender()
{
	ImGui::Begin( "Test" );
	ImGui::Text( "Hi" );
	ImGui::End();
}
