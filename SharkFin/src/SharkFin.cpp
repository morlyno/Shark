
#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Shark {

	class SharkFin : public Application
	{
	public:
		SharkFin()
		{
			PushLayer(new EditorLayer());
		}

		~SharkFin()
		{
		}

	};

	Application* CreateApplication()
	{
		return new SharkFin();
	}

}