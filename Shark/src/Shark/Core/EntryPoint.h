#pragma once

extern Shark::Application* Shark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Shark::Core::Initialize();

	auto app = Shark::CreateApplication(argc, argv);
	app->Run();
	delete app;

	Shark::Core::Shutdown();

	return 0;
}
