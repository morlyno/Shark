#pragma once

#include "Core.h"

namespace Shark {

	class SHARK_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();

}

