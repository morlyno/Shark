#include "skpch.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

namespace Shark {

	SelectionChangedEvent::SelectionChangedEvent(Entity entity)
		: m_EntityID(entity), m_Scene(entity.GetScene())
	{
	}

	Entity SelectionChangedEvent::GetSelectedEntity()
	{
		Entity entity{ (entt::entity)m_EntityID, m_Scene };
		return entity;
	}

}