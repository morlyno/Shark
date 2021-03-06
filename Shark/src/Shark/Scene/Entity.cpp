#include "skpch.h"
#include "Entity.h"

namespace Shark {

	Entity::Entity(uint32_t entityhandle, const Weak<Scene>& scene)
		: m_EntityHandle((entt::entity)entityhandle), m_Scene(scene)
	{
	}

	Entity::Entity(entt::entity entityhandle, const Weak<Scene>& scene)
		: m_EntityHandle(entityhandle), m_Scene(scene)
	{
	}

}