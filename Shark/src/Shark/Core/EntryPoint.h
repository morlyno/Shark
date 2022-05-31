#pragma once

#include "Shark/Debug/Instrumentor.h"

extern Shark::Application* Shark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Shark::Log::Init();

	auto app = Shark::CreateApplication(argc, argv);
	app->Run();
	delete app;

	Shark::Log::Shutdown();

	SK_PROFILE_SHUTDOWN();

	return 0;
}
