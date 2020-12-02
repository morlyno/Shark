#pragma once
#include <Shark.h>
#include <imgui.h>

class TestLayer : public Shark::Layer
{
public:
	TestLayer( const std::string& name = std::string{} )
		:
		Layer( "TestLayer" + name )
	{
	}

	void OnUpdate() override
	{
	}

	void OnEvent( Shark::Event& e ) override
	{
	}

	void OnImGuiRender() override
	{
		if ( ImGui::Begin( "Test" ) )
		{
			ImGui::Text( "Test" );
		}
		ImGui::End();

		static bool open = true;
		ImGui::ShowDemoWindow( &open );
	}
};
