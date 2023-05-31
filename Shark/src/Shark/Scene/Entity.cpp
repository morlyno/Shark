#include "skpch.h"
#include "Entity.h"

#include "Shark/Debug/enttDebug.h"

namespace Shark {

	Entity::Entity(uint32_t entityhandle, Weak<Scene> scene)
		: m_EntityHandle((entt::entity)entityhandle), m_Scene(scene)
	{
	}

	Entity::Entity(entt::entity entityhandle, Weak<Scene> scene)
		: m_EntityHandle(entityhandle), m_Scene(scene)
	{
	}

	void Entity::SetParent(Entity parent)
	{
		if (!parent && HasParent())
			RemoveParent();

		if (HasParent())
			RemoveParent();

		GetComponent<RelationshipComponent>().Parent = parent.GetUUID();
		parent.GetComponent<RelationshipComponent>().Children.emplace_back(GetUUID());
	}

	void Entity::AddChild(Entity child)
	{
		SK_CORE_ASSERT(!HasChild(child.GetUUID()));
		if (HasChild(child.GetUUID()))
			return;

		GetComponent<RelationshipComponent>().Children.emplace_back(child.GetUUID());
		child.GetComponent<RelationshipComponent>().Parent = GetUUID();
	}

	void Entity::RemoveParent()
	{
		auto& relShip = GetComponent<RelationshipComponent>();
		if (!relShip.Parent.IsValid())
			return;

		RemoveTargetFromParent(*this);
		relShip.Parent = UUID::Null;
	}

	void Entity::RemoveChild(UUID childID)
	{
		Entity childEntity = m_Scene.GetRef()->GetEntityByUUID(childID);
		RemoveTargetFromParent(childEntity);
		childEntity.GetComponent<RelationshipComponent>().Parent = UUID::Null;
	}

	void Entity::RemoveChildren()
	{
		for (UUID childID : Children())
		{
			Entity child = m_Scene.GetRef()->GetEntityByUUID(childID);
			child.GetComponent<RelationshipComponent>().Parent = UUID::Null;
		}
		Children().clear();
	}

	bool Entity::HasChild(UUID childID)
	{
		auto& children = Children();
		for (const auto& id : children)
			if (id == childID)
				return true;
		return false;
	}

	void Entity::RemoveTargetFromParent(Entity target)
	{
		UUID targetID = target.GetUUID();
		Entity parentEntity = target.m_Scene.GetRef()->GetEntityByUUID(target.ParentUUID());

		auto& children = parentEntity.Children();
		for (size_t i = 0; i < children.size(); i++)
		{
			if (children[i] == targetID)
			{
				children.erase(children.begin() + i);
				return;
			}
		}

		SK_CORE_ERROR("Invalid Parent-Child Relationship");
		SK_CORE_ASSERT(false, "Invalid Parent-Child Relationship");
	}

}

