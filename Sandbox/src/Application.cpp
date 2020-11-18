
#include <Shark.h>

class Sandbox : public Shark::Application
{
public:
	Sandbox()
	{
	}

	~Sandbox()
	{
	}

};

Shark::Application* Shark::CreateApplication()
{
	return new Sandbox;
}