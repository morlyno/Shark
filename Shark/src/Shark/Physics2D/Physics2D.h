#pragma once

#include "Shark/Physics2D/PhysicsScene.h"

namespace Shark {

	class Scene;
	class Entity;

	class Physics2D
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<Physics2DScene> CreateScene(Ref<Scene> scene);
		static void ReleaseScene();

		static Ref<Physics2DScene> GetScene();
	};

}
