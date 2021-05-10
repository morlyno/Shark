
#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Shark {

	class SharkFin : public Application
	{
	public:
		SharkFin(int argc, char** argv)
			: Application(argc, argv)
		{
			PushLayer(new EditorLayer());
		}

		~SharkFin()
		{
		}

	};

	Application* CreateApplication(int argc, char** argv)
	{
		RendererAPI::SetAPI(RendererAPI::API::DirectX11);
		return new SharkFin(argc, argv);
	}

}