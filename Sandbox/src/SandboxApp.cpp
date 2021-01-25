
#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "TestLayer.h"
#include "Sandbox2D.h"

class Sandbox : public Shark::Application
{
public:
	Sandbox()
		:
		//layer(new TestLayer("TextLayer"))
		layer(new Sandbox2D("Sandbox2D"))
	{
		PushLayer(layer);
	}

	~Sandbox()
	{
	}

private:
	Shark::Layer* layer;

};

Shark::Application* Shark::CreateApplication()
{
	return new Sandbox;
}