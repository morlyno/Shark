#pragma once

#include "Shark/Scripting/ScriptTypes.h"

namespace Shark {

	struct EntityScript
	{
		uint64_t ScriptID = 0;
		Coral::ManagedObject Instance;
		std::unordered_map<uint64_t, FieldStorage> Fields;
	};

	class ScriptStorage
	{
	public:
		void SetupEntityStorage(uint64_t scriptID, UUID entityID);
		void RemoveEntityStorage(uint64_t scriptID, UUID entityID);
		void CopyEntityStorage(UUID sourceEntityID, UUID targetEntityID, ScriptStorage& targetStorage);

		void CopyTo(ScriptStorage& storage);

		void InitializeFieldStorage(EntityScript& storage, uint64_t fieldID, const FieldMetadata& metadata);
	public:
		std::unordered_map<UUID, EntityScript> EntityInstances;
	};

}
