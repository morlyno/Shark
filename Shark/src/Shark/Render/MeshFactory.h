#pragma once

#include "Shark/Render/Mesh.h"

namespace Shark {

	class MeshFactory
	{
	public:
		static Ref<MeshSource> CreateCube();
		static Ref<MeshSource> CreateSphere(int latDiv = 12, int longDiv = 24);

		static Ref<MeshSource> GetCube();
		static Ref<MeshSource> GetSphere();
	};

}
