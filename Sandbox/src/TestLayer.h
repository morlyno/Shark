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

	void OnUpdate( Shark::TimeStep t ) override
	{
	}

	void OnEvent( Shark::Event& e ) override
	{
	}

	void OnImGuiRender() override
	{
		ImGui::Begin( "Test" );
		ImGui::Text( "Hi" );
		ImGui::End();
	}
};
