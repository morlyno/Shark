#include "skpch.h"
#include "Entity.h"

namespace Shark {

	Entity::Entity(uint32_t entityhandle, const Weak<Scean>& scean)
		: m_EntityHandle((entt::entity)entityhandle), m_Scean(scean)
	{
	}

	Entity::Entity(entt::entity entityhandle, const Weak<Scean>& scean)
		: m_EntityHandle(entityhandle), m_Scean(scean)
	{
	}

}