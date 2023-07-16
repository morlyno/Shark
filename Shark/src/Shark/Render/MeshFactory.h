#pragma once

#include "Shark/Render/Mesh.h"

namespace Shark {

	class MeshFactory
	{
	public:
		static Ref<Mesh> CreateCube();
	};

}
