#include "skfpch.h"

#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorLayer.h"
#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	class SharkFin : public Application
	{
	public:
		SharkFin(int argc, char** argv)
			: Application(argc, argv)
		{
			SK_PROFILE_FUNCTION();

			PushLayer(new EditorLayer());
		}

		~SharkFin()
		{
			SK_PROFILE_FUNCTION();
		}

	};

	Application* CreateApplication(int argc, char** argv)
	{
		SK_PROFILE_FUNCTION();

		RendererAPI::SetAPI(RendererAPI::API::DirectX11);
		return new SharkFin(argc, argv);
	}

}