#pragma once

extern Shark::Application* Shark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	auto app = Shark::CreateApplication(argc, argv);
	app->Run();
	delete app;

	return 0;
}
