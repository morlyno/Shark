#pragma once

#ifdef SK_PLATFORM_WINDOWS

extern Shark::Application* Shark::CreateApplication();

int main( int argc,char** argv )
{
	Shark::Log::Init();

	SK_CORE_LOG_TRACE( "traze" );
	SK_CORE_LOG_INFO( "info" );
	SK_CORE_LOG_WARN( "warn" );
	SK_CORE_LOG_ERROR( "error" );
	SK_CORE_LOG_CRITICAL( "critical" );
	SK_CORE_LOG_DEBUG( "debug" );
	SK_CLIENT_LOG_TRACE( "traze" );
	SK_CLIENT_LOG_INFO( "info" );
	SK_CLIENT_LOG_WARN( "warn" );
	SK_CLIENT_LOG_ERROR( "error" );
	SK_CLIENT_LOG_CRITICAL( "critical" );
	SK_CLIENT_LOG_DEBUG( "debug" );

	auto app = Shark::CreateApplication();
	app->Run();
	delete app;
}

#endif