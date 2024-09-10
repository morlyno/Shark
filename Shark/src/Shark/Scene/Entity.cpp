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
		if (!parent || HasParent())
			RemoveParent();

		GetComponent<RelationshipComponent>().Parent = parent.GetUUID();
		RemoveComponent<Internal::RootParentComponent>();
		parent.GetComponent<RelationshipComponent>().Children.emplace_back(GetUUID());
	}

	void Entity::AddChild(Entity child)
	{
		SK_CORE_ASSERT(!HasChild(child.GetUUID()));
		if (HasChild(child.GetUUID()))
			return;

		GetComponent<RelationshipComponent>().Children.emplace_back(child.GetUUID());
		child.GetComponent<RelationshipComponent>().Parent = GetUUID();
		child.RemoveComponent<Internal::RootParentComponent>();
	}

	void Entity::RemoveParent()
	{
		auto& relShip = GetComponent<RelationshipComponent>();
		if (!relShip.Parent)
			return;

		RemoveTargetFromParent(*this);
		relShip.Parent = UUID::Invalid;
		AddComponent<Internal::RootParentComponent>();
	}

	void Entity::RemoveChild(UUID childID)
	{
		Entity childEntity = m_Scene.GetRef()->TryGetEntityByUUID(childID);
		RemoveTargetFromParent(childEntity);
		childEntity.GetComponent<RelationshipComponent>().Parent = UUID::Invalid;
		childEntity.AddComponent<Internal::RootParentComponent>();
	}

	void Entity::RemoveChildren()
	{
		for (UUID childID : Children())
		{
			Entity child = m_Scene.GetRef()->TryGetEntityByUUID(childID);
			child.GetComponent<RelationshipComponent>().Parent = UUID::Invalid;
			child.AddComponent<Internal::RootParentComponent>();
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
		Entity parentEntity = target.m_Scene.GetRef()->TryGetEntityByUUID(target.ParentUUID());

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

