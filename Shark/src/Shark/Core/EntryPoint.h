
#pragma once

#ifdef SK_PLATFORM_WINDOWS

extern Shark::Application* Shark::CreateApplication();

int main(int argc, char** argv)
{
	Shark::Log::Init();
	SK_CORE_INFO("Log Init");

	auto app = Shark::CreateApplication();
	auto r = app->Run();
	delete app;
	return r;
}

#endif