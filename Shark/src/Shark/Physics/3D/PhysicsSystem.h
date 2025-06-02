#pragma once

#include "Shark/Physics/3D/PhysicsScene.h"
#include "Shark/Physics/3D/CookingFactory.h"

namespace Shark {

	class Scene;

	class PhysicsSystem
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<PhysicsScene> CreateScene(Ref<Scene> scene);
		static void CookMesh(AssetHandle meshHandle, bool forceCook = false);

		static CookingFactory& GetCookingFactory();
		static MeshColliderCache& GetColliderCache();

		static JPH::TempAllocator* GetAllocator();
		static JPH::JobSystem* GetJobSystem();
	};

}
