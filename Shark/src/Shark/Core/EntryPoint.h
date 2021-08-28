#pragma once

#include "Shark/Debug/Instrumentor.h"

extern Shark::Application* Shark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Shark::Log::Init();

	SK_PROFILE_BEGIN_SESSION("Start Up", "StartUp.json");
	auto app = Shark::CreateApplication(argc, argv);
	SK_PROFILE_END_SESSION();

	SK_PROFILE_BEGIN_SESSION("Rutime", "Runtime.json");
	app->Run();
	SK_PROFILE_END_SESSION();

	SK_PROFILE_BEGIN_SESSION("Shut Down", "ShutDown.json");
	delete app;
	SK_PROFILE_END_SESSION();

	return 0;
}
