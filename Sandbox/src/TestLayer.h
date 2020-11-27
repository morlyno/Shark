#pragma once
#include <Shark.h>

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
};
