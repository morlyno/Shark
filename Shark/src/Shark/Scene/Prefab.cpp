#include "skpch.h"
#include "Prefab.h"

namespace Shark {

	Prefab::Prefab()
	{
		m_Scene = Ref<Scene>::Create();
	}

	Prefab::Prefab(Entity entity)
	{
		m_Scene = Ref<Scene>::Create();
		Create(entity);
	}

	Prefab::~Prefab()
	{

	}

	void Prefab::Create(Entity entity)
	{
		m_Scene->DestroyEntities();

		Ref<Scene> sourceContext = entity.m_Scene.GetRef();
		Entity rootEntity = CopyHierarchy({}, sourceContext, entity);
		rootEntity.GetComponent<PrefabComponent>().IsRoot = true;
		rootEntity.GetComponent<RelationshipComponent>().Parent = UUID::Invalid;
		rootEntity.Transform().Translation = glm::vec3(0.0f);
		rootEntity.Transform().Rotation = glm::vec3(0.0f);
		rootEntity.Transform().Scale = glm::vec3(1.0f);
		m_RootEntityID = rootEntity.GetUUID();
	}

	Entity Prefab::GetRootEntity() const
	{
		return m_Scene->TryGetEntityByUUID(m_RootEntityID);
	}

	bool Prefab::HasValidRoot() const
	{
		return m_Scene->IsValidEntityID(m_RootEntityID);
	}

	void Prefab::SetRootEntity(Entity entity)
	{
		if (!m_Scene->IsValidEntity(entity))
			return;

		if (m_Scene->IsValidEntityID(m_RootEntityID))
		{
			Entity rootEntity = m_Scene->GetEntityByID(m_RootEntityID);
			rootEntity.GetComponent<PrefabComponent>().IsRoot = false;
		}

		m_RootEntityID = entity.GetUUID();
		entity.GetComponent<PrefabComponent>().IsRoot = true;
	}

	void Prefab::ClearRootEntity()
	{
		if (!m_Scene->IsValidEntityID(m_RootEntityID))
			return;

		Entity rootEntity = m_Scene->GetEntityByID(m_RootEntityID);
		rootEntity.GetComponent<PrefabComponent>().IsRoot = false;
	}

	Entity Prefab::CopyHierarchy(Entity parent, Ref<Scene> sourceContext, Entity sourceEntity)
	{
		SK_CORE_ASSERT(sourceContext->IsEditorScene());
		Entity entity = m_Scene->CreateChildEntityWithUUID(parent, sourceEntity.GetUUID(), sourceEntity.Tag());
		entity.AddComponent<PrefabComponent>(Handle, entity.GetUUID());
		
		entity.AddOrReplaceComponent<TransformComponent>(sourceEntity.GetComponent<TransformComponent>());

		ForEach(AllComponents::Except<CoreComponents, PrefabComponent>{}, [entity, sourceEntity]<typename TComponent>() mutable
		{
			if (TComponent* component = sourceEntity.TryGetComponent<TComponent>())
				entity.AddOrReplaceComponent<TComponent>(*component);
		});

		if (entity.HasComponent<ScriptComponent>())
		{
			const auto& scriptComponent = entity.GetComponent<ScriptComponent>();
			ScriptStorage& storage = m_Scene->GetScriptStorage();
			storage.SetupEntityStorage(scriptComponent.ScriptID, entity.GetUUID());
			sourceContext->GetScriptStorage().CopyEntityStorage(sourceEntity.GetUUID(), entity.GetUUID(), storage);
		}

		for (UUID childID : sourceEntity.Children())
		{
			Entity child = sourceContext->GetEntityByID(childID);
			CopyHierarchy(entity, sourceContext, child);
		}

		return entity;
	}

}
