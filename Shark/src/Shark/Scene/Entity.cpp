#include "skpch.h"
#include "Entity.h"

#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/TagComponent.h"

namespace Shark {

	Entity::Entity(uint32_t entityhandle, const Weak<Scene>& scene)
		: m_EntityHandle((entt::entity)entityhandle), m_Scene(scene)
	{
	}

	Entity::Entity(entt::entity entityhandle, const Weak<Scene>& scene)
		: m_EntityHandle(entityhandle), m_Scene(scene)
	{
	}

	TransformComponent& Entity::GetTransform()
	{
		return GetComponent<TransformComponent>();
	}

}

