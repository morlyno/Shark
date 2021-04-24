#include "skpch.h"
#include "Shark/Scean/Scean.h"
#include "Shark/Scean/Entity.h"

namespace Shark {

	SelectionChangedEvent::SelectionChangedEvent(Entity entity)
		: m_EntityID(entity), m_Scean(entity.GetScean())
	{
	}

	Entity SelectionChangedEvent::GetSelectedEntity()
	{
		Entity entity{ (entt::entity)m_EntityID, m_Scean };
		return entity;
	}

}