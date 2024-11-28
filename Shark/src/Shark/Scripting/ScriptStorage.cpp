#include "skpch.h"
#include "ScriptStorage.h"

#include "Shark/Scripting/ScriptEngine.h"

namespace Shark {

	void ScriptStorage::SetupEntityStorage(uint64_t scriptID, UUID entityID)
	{
		auto& scriptEngine = ScriptEngine::Get();
		SK_CORE_VERIFY(scriptEngine.IsValidScriptID(scriptID));
		SK_CORE_VERIFY(!EntityInstances.contains(entityID));

		auto& entityStorage = EntityInstances[entityID];
		entityStorage.ScriptID = scriptID;

		const auto& scriptMetadata = scriptEngine.GetScriptMetadata(scriptID);

		for (const auto& [id, metadata] : scriptMetadata.Fields)
		{
			InitializeFieldStorage(entityStorage, id, metadata);
		}
	}

	void ScriptStorage::RemoveEntityStorage(uint64_t scriptID, UUID entityID)
	{
		auto& scriptEngine = ScriptEngine::Get();
		SK_CORE_VERIFY(scriptEngine.IsValidScriptID(scriptID));
		SK_CORE_VERIFY(EntityInstances.contains(entityID));

		auto& entityStorage = EntityInstances.at(entityID);
		for (auto& [fieldID, storage] : entityStorage.Fields)
			storage.m_ValueBuffer.Release();

		EntityInstances.erase(entityID);
	}

	void ScriptStorage::CopyEntityStorage(UUID sourceEntityID, UUID targetEntityID, ScriptStorage& targetStorage)
	{
		auto& scriptEngine = ScriptEngine::Get();
		if (!targetStorage.EntityInstances.contains(targetEntityID))
		{
			SK_CORE_ERROR_TAG("Scripting", "Cannot copy script storage to entity '{}' because its storage isn't setup jet.", sourceEntityID);
			return;
		}

		const auto& sourceStorage = EntityInstances.at(sourceEntityID);

		if (!scriptEngine.IsValidScriptID(sourceStorage.ScriptID))
		{
			SK_CORE_ERROR_TAG("Scripting", "Cannot copy script storage from entity '{}' because the script ID '{}' is no longer valid.", sourceEntityID, sourceStorage.ScriptID);
			return;
		}

		auto& destinationStorage = targetStorage.EntityInstances.at(targetEntityID);

		if (destinationStorage.ScriptID != sourceStorage.ScriptID)
		{
			SK_CORE_ERROR_TAG("Scripting", "Cannot copy from entity '{}' to entity '{}' because they have different scripts.", sourceEntityID, targetEntityID);
			return;
		}

		const auto& scriptMetadata = scriptEngine.GetScriptMetadata(sourceStorage.ScriptID);

		for (auto& [fieldID, sourceField] : sourceStorage.Fields)
		{
			if (!scriptMetadata.Fields.contains(fieldID))
			{
				SK_CORE_ERROR_TAG("Scripting", "Cannot copy field storage for field {}! The script no longer contains this field.", sourceField.GetName());
				return;
			}

			auto& destinationField = destinationStorage.Fields[fieldID];
			destinationField.m_Name = sourceField.m_Name;
			destinationField.m_DataType = sourceField.m_DataType;
			destinationField.m_ValueBuffer = Buffer::Copy(sourceField.m_ValueBuffer);
			destinationField.m_Instance = nullptr;
		}

	}

	void ScriptStorage::CopyTo(ScriptStorage& storage)
	{
		for (const auto& [entityID, entityScript] : EntityInstances)
		{
			storage.SetupEntityStorage(entityScript.ScriptID, entityID);
			CopyEntityStorage(entityID, entityID, storage);
		}
	}

	void ScriptStorage::InitializeFieldStorage(EntityScript& storage, uint64_t fieldID, const FieldMetadata& metadata)
	{
		auto& fieldStorage = storage.Fields[fieldID];
		fieldStorage.m_Name = metadata.Name;
		fieldStorage.m_DataType = metadata.DataType;
		fieldStorage.m_Instance = nullptr;

		if (metadata.DefaultValue)
			fieldStorage.m_ValueBuffer = Buffer::Copy(metadata.DefaultValue);
		else
		{
			fieldStorage.m_ValueBuffer.Allocate(GetDataTypeSize(metadata.DataType));
			fieldStorage.m_ValueBuffer.SetZero();
		}
	}

}
