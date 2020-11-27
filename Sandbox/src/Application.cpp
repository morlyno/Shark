
#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "TestLayer.h"

class Sandbox : public Shark::Application
{
public:
	Sandbox()
		:
		layer( new TestLayer() )
	{
		AddLayer( layer );
	}

	~Sandbox()
	{
	}

private:
	TestLayer* layer;

};

Shark::Application* Shark::CreateApplication()
{
	return new Sandbox;
}