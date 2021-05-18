#pragma once

extern Shark::Application* Shark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Shark::Application* app = nullptr;

	{
		Shark::Timer timer;
		app = Shark::CreateApplication(argc, argv);
		SK_CORE_TRACE("Startup tock: {0}ms", timer.Stop());
	}

	app->Run();

	{
		Shark::Timer timer;
		delete app;
		SK_CORE_TRACE("Destroction tock: {0}ms", timer.Stop());
	}

	return 0;
}
