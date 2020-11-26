
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