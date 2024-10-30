#include "skpch.h"
#include "Entity.h"

#include "Shark/Debug/enttDebug.h"

namespace Shark {

	Entity::Entity(entt::entity entityhandle, Weak<Scene> scene)
		: m_EntityHandle(entityhandle), m_Scene(scene)
	{
	}

	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && !m_Scene.Expired() && m_Scene.GetRef()->m_Registry.valid(m_EntityHandle);
	}

	Entity Entity::Parent() const
	{
		return m_Scene.GetRef()->TryGetEntityByUUID(ParentID());
	}

	UUID Entity::ParentID() const
	{
		return GetComponent<RelationshipComponent>().Parent;
	}

	std::vector<Shark::UUID>& Entity::Children()
	{
		return GetComponent<RelationshipComponent>().Children;
	}

	const std::vector<Shark::UUID>& Entity::Children() const
	{
		return GetComponent<RelationshipComponent>().Children;
	}

	bool Entity::HasParent() const
	{
		return ParentID() != UUID::Invalid;
	}

	bool Entity::HasChild(UUID childID) const
	{
		const auto& children = Children();
		return std::ranges::find(children, childID) != children.end();
	}

	bool Entity::HasChildren() const
	{
		return !Children().empty();
	}

	void Entity::SetParent(Entity parent)
	{
		if (HasParent())
		{
			Parent().RemoveChild(*this);
		}

		if (!parent)
			return;

		GetComponent<RelationshipComponent>().Parent = parent.GetUUID();
		parent.Children().push_back(GetUUID());
		//RemoveComponentIsExists<Internal::RootParentComponent>();
	}

	void Entity::AddChild(Entity child)
	{
		if (!child)
			return;

		child.SetParent(*this);
		Children().push_back(child.GetUUID());
	}

	void Entity::RemoveChild(Entity child)
	{
		if (!child)
			return;

		std::erase(Children(), child.GetUUID());
		child.GetComponent<RelationshipComponent>().Parent = UUID::Invalid;
		//child.AddComponent<Internal::RootParentComponent>();
	}

	bool Entity::IsAncestorOf(Entity entity) const
	{
		return entity.IsDescendantOf(*this);
	}

	bool Entity::IsDescendantOf(Entity entity) const
	{
		// this child of entity

		Entity parent = Parent();
		if (!parent)
			return false;

		if (parent == entity)
			return true;

		return parent.IsDescendantOf(entity);
	}

}

