#pragma once

#include "Shark/Physics/3D/PhysicsScene.h"

namespace Shark {

	class Scene;

	class PhysicsSystem
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<PhysicsScene> CreateScene(Ref<Scene> scene);

		static JPH::TempAllocator* GetAllocator();
		static JPH::JobSystem* GetJobSystem();
	};

}
