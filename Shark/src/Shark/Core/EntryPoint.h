#pragma once

extern Shark::Application* Shark::CreateApplication();

int main(int argc, char** argv)
{
	Shark::Log::Init();
	SK_CORE_INFO("Log Init");

	auto app = Shark::CreateApplication();
	app->Run();
	delete app;

	return 0;
}
