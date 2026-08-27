#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Asset/AssetTypes.h"

namespace Coral {
	class ManagedObject;
}

namespace Shark {

	struct PrefabComponent
	{
		AssetHandle Prefab;
		UUID EntityID;
		bool IsRoot = false;

		PrefabComponent() = default;
		PrefabComponent(AssetHandle prefab, UUID entityID)
			: Prefab(Prefab), EntityID(entityID) {
		}
	};

	struct ScriptComponent
	{
		uint64_t ScriptID = 0;
		Coral::ManagedObject* Instance = nullptr;
		bool OnCreateCalled = false;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	struct AudioComponent
	{
		AssetHandle Audio = AssetHandle::Invalid;
		bool        PlayOnWake = false;
		float       VolumeMultiplier = 1.0f;
		float       PitchMultiplier = 1.0f;

		AudioComponent() = default;
		AudioComponent(const AudioComponent&) = default;
	};

	struct AnimationComponent
	{
		AssetHandle Animation;
		bool Loop   = true;
		bool Update = true;

		AnimationComponent() = default;
		AnimationComponent(const AnimationComponent&) = default;
	};

}
