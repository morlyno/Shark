#pragma once

#ifdef SK_PLATFORM_WINDOWS

extern Shark::Application* Shark::CreateApplication();

int main( int argc,char** argv )
{
	auto app = Shark::CreateApplication();
	app->Run();
	delete app;
}

#endif