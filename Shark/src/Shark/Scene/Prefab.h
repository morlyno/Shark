#pragma once

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

namespace Shark {

	class Prefab : public Asset
	{
	public:
		Prefab();
		Prefab(Entity entity);
		~Prefab();

		void Create(Entity entity);
		Ref<Scene> GetScene() const { return m_Scene; }
		UUID GetRootEntityID() const { return m_RootEntityID; }
		Entity GetRootEntity() const;

		bool ShouldSetActiveCamera() const { return m_SetActiveCamera; }
		UUID GetActiveCameraID() const { return m_Scene->GetActiveCameraUUID(); }

	public:
		bool HasValidRoot() const;
		void SetRootEntity(Entity entity);
		void ClearRootEntity();

	private:
		Entity CopyHierarchy(Entity parent, Ref<Scene> sourceContext, Entity sourceEntity);

	public:
		static AssetType GetStaticType() { return AssetType::Prefab; }
		virtual AssetType GetAssetType() const { return GetStaticType(); }
	private:
		Ref<Scene> m_Scene;
		UUID m_RootEntityID;

		bool m_SetActiveCamera = true;

		friend class PrefabSerializer;
		friend class PrefabEditorPanel;
		friend class Scene;
	};

}
