#include "skpch.h"
#include "Physics2D.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

namespace Shark {

	static Ref<Physics2DScene> s_PhysicsScene = nullptr;

	void Physics2D::Initialize()
	{
	}

	void Physics2D::Shutdown()
	{
		s_PhysicsScene = nullptr;
	}

	Ref<Physics2DScene> Physics2D::CreateScene(Ref<Scene> scene)
	{
		s_PhysicsScene = Ref<Physics2DScene>::Create(scene);
		return s_PhysicsScene;
	}

	void Physics2D::ReleaseScene()
	{
		s_PhysicsScene = nullptr;
	}

	Ref<Physics2DScene> Physics2D::GetScene()
	{
		return s_PhysicsScene;
	}

}

