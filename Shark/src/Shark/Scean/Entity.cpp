#include "skpch.h"
#include "Entity.h"

namespace Shark {

	Entity::Entity(entt::entity entityhandle, const WeakRef<Scean>& scean)
		: m_EntityHandle(entityhandle), m_Scean(scean)
	{
	}

}