#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"
#include "Shark/Scripting/ScriptGlue.h"
#include "Shark/File/FileSystem.h"

#include <Coral/FieldInfo.hpp>
#include <Coral/TypeCache.hpp>

namespace Shark {

	static void OnCoralMessage(std::string_view message, Coral::MessageLevel level)
	{
		switch (level)
		{
			case Coral::MessageLevel::Trace: SK_CORE_TRACE_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Info: SK_CORE_INFO_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Warning: SK_CORE_WARN_TAG("Scripting", "{}", message); break;
			case Coral::MessageLevel::Error: SK_CORE_ERROR_TAG("Scripting", "{}", message); break;
		}
	}

	static void OnCSException(std::string_view message)
	{
		SK_CONSOLE_ERROR("C# Exception: {}", message);
	}

	void ScriptEngine::InitializeHost()
	{
		Coral::HostSettings settings;
		// #TODO(moro): maybe copy file to Shark-Editor/...
		settings.CoralDirectory = FileSystem::Absolute("DotNet").string();
		settings.MessageCallback = OnCoralMessage;
		settings.ExceptionCallback = OnCSException;

		m_Host = Scope<Coral::HostInstance>::Create();
		Coral::CoralInitStatus status = m_Host->Initialize(settings);

		if (status == Coral::CoralInitStatus::Success)
		{
			SK_CORE_INFO_TAG("Scripting", "Coral host initialized");
			return;
		}

		// TODO(moro): better error handling
		SK_CORE_ERROR_TAG("Scripting", "{}", status);
	}

	void ScriptEngine::ShutdownHost()
	{
		m_Host->Shutdown();
		m_Host = nullptr;
	}

	void ScriptEngine::InitializeCore(Ref<Project> project)
	{
		m_Project = project;
		m_LoadContext = Scope<Coral::AssemblyLoadContext>::Create(m_Host->CreateAssemblyLoadContext("Shark-Load-Context"));

		m_CoreAssembly = &m_LoadContext->LoadAssembly("Resources/Binaries/Shark-ScriptCore.dll");
		SK_CORE_DEBUG_TAG("Scripting", "Core assembly loaded with status {}", m_CoreAssembly->GetLoadStatus());
		
		ScriptGlue::Initialize(*m_CoreAssembly);
	}

	void ScriptEngine::ShutdownCore()
	{
		ScriptGlue::Shutdown();

		m_CoreAssembly = nullptr;
		m_AppAssembly = nullptr;
		m_Host->UnloadAssemblyLoadContext(*m_LoadContext);
		m_LoadContext = nullptr;

		for (auto& [scriptID, scriptMetadata] : m_ScriptMetadata)
			for (auto& [fieldID, fieldMetadata] : scriptMetadata.Fields)
				fieldMetadata.DefaultValue.Release();

		m_ScriptMetadata.clear();

		Coral::TypeCache::Get().Clear();
		m_Project = nullptr;
	}

	void ScriptEngine::LoadAppAssembly()
	{
		auto assemblyPath = m_Project->GetScriptModulePath();
		
		m_AppAssembly = &m_LoadContext->LoadAssembly(assemblyPath);
		SK_CORE_DEBUG_TAG("Scripting", "App assembly loaded with status {}", m_AppAssembly->GetLoadStatus());

		BuildScriptCache();
		SK_CONSOLE_INFO("App assembly loaded\nStatus: {}", m_AppAssembly->GetLoadStatus());
	}

	void ScriptEngine::ReloadAssemblies()
	{
		Ref<Project> project = m_Project;
		ShutdownCore();
		InitializeCore(project);
		LoadAppAssembly();
	}

	bool ScriptEngine::IsValidScriptID(uint64_t scriptID)
	{
		if (!m_ScriptMetadata.contains(scriptID))
			return false;

		Coral::Type* type = m_ScriptMetadata.at(scriptID).Type;
		return type && *type;
	}

	static const std::unordered_map<std::string_view, ManagedFieldType> s_DataTypeLookupTable = {
		{ "System.Boolean", ManagedFieldType::Bool },
		{ "System.Byte", ManagedFieldType::Byte },
		{ "System.SByte", ManagedFieldType::SByte },
		{ "System.Int16", ManagedFieldType::Short },
		{ "System.UInt16", ManagedFieldType::UShort },
		{ "System.Int32", ManagedFieldType::Int },
		{ "System.UInt32", ManagedFieldType::UInt },
		{ "System.Int64", ManagedFieldType::Long },
		{ "System.UInt64", ManagedFieldType::ULong },
		{ "System.Single", ManagedFieldType::Float },
		{ "System.Double", ManagedFieldType::Double },
		{ "System.String", ManagedFieldType::String },

		{ "Shark.Entity", ManagedFieldType::Entity },
		{ "Shark.Vector2", ManagedFieldType::Vector2 },
		{ "Shark.Vector3", ManagedFieldType::Vector3 },
		{ "Shark.Vector4", ManagedFieldType::Vector4 }
	};

	void ScriptEngine::BuildScriptCache()
	{
		SK_CORE_ASSERT(m_ScriptMetadata.empty());

		Coral::Type& entityType = m_AppAssembly->GetType("Shark.Entity");
		Coral::Type& componentType = m_AppAssembly->GetType("Shark.Component");

		Coral::Type& showInEditorAttribute = m_CoreAssembly->GetType("Shark.ShowInEditorAttribute");
		Coral::Type& hideFromEditorAttribute = m_CoreAssembly->GetType("Shark.HideFromEditorAttribute");

		SK_CORE_VERIFY((bool)showInEditorAttribute);
		SK_CORE_VERIFY((bool)hideFromEditorAttribute);

		auto& types = m_AppAssembly->GetTypes();
		for (Coral::Type* type : types)
		{
			if (!type->IsSubclassOf(entityType))
				continue;

			Coral::String fullNameMS = type->GetFullName();
			std::string fullName = fullNameMS;
			uint64_t hash = Hash::GenerateFNV(fullName);
			m_ScriptMetadata[hash] = { fullName, type };


			Coral::ManagedObject tempInstance = type->CreateInstance();
			ScriptMetadata& scriptMetadata = m_ScriptMetadata.at(hash);

			auto fields = type->GetFields();
			for (auto& fieldInfo : fields)
			{
				Coral::ScopedString nameMS = fieldInfo.GetName();
				uint64_t fieldHash = Hash::GenerateFNV(fieldHash);
				Coral::Type& fieldType = fieldInfo.GetType();
				Coral::ScopedString typeNameMS = fieldType.GetFullName();

				std::string name = nameMS;
				std::string typeName = typeNameMS;

				bool isDerivedComponent = false;
				if (!s_DataTypeLookupTable.contains(typeName))
					continue;

				if (fieldInfo.HasAttribute(hideFromEditorAttribute))
					continue;

				Coral::TypeAccessibility accessibility = fieldInfo.GetAccessibility();
				if (!fieldInfo.HasAttribute(showInEditorAttribute) && accessibility != Coral::TypeAccessibility::Public)
					continue;

				Buffer defaultValue;
				ManagedFieldType dataType = s_DataTypeLookupTable.at(typeName);

				// #TODO(moro): Arrays
				switch (dataType)
				{
					case ManagedFieldType::Bool:
					case ManagedFieldType::Byte:
					case ManagedFieldType::SByte:
					case ManagedFieldType::Short:
					case ManagedFieldType::UShort:
					case ManagedFieldType::Int:
					case ManagedFieldType::UInt:
					case ManagedFieldType::Long:
					case ManagedFieldType::ULong:
					case ManagedFieldType::Float:
					case ManagedFieldType::Double:
					case ManagedFieldType::Vector2:
					case ManagedFieldType::Vector3:
					case ManagedFieldType::Vector4:
					{
						defaultValue.Allocate(fieldType.GetSize());
						tempInstance.GetFieldValueRaw(name, defaultValue.Data);
						break;
					}

					case ManagedFieldType::String:
					{
						Coral::String managedString = tempInstance.GetFieldValue<Coral::String>(name);
						if (managedString.Data())
						{
							std::string str = managedString;
							defaultValue.Allocate(str.size());
							defaultValue.Write(str.data(), str.size());
						}
						Coral::String::Free(managedString);
						break;
					}

					// nothing to do for those types (no default value possible)
					case ManagedFieldType::Entity:
					{
						defaultValue.Allocate(sizeof(UUID));
						defaultValue.SetZero();
						break;
					}

				}

				scriptMetadata.Fields[Hash::GenerateFNV(name)] = { name, type, dataType, defaultValue };
			}

			tempInstance.Destroy();
		}
	}

	ScriptEngine& ScriptEngine::Get()
	{
		return Application::Get().GetScriptEngine();
	}

	Coral::ManagedObject* ScriptEngine::Instantiate(UUID entityID, ScriptStorage& storage)
	{
		if (!m_CurrentScene->IsValidEntityID(entityID))
			return nullptr;

		auto& entityStorage = storage.EntityInstances.at(entityID);

		Entity entity = m_CurrentScene->GetEntityByID(entityID);
		auto& scriptComponent = entity.GetComponent<ScriptComponent>();

		// Invalid Script
		if (!IsValidScriptID(scriptComponent.ScriptID))
			return nullptr;

		const auto& metadata = m_ScriptMetadata.at(scriptComponent.ScriptID);
		entityStorage.Instance = metadata.Type->CreateInstance((uint64_t)entityID);
		scriptComponent.Instance = &entityStorage.Instance;

		Coral::ManagedObject& instance = entityStorage.Instance;
		for (auto& [fieldID, fieldStorage] : entityStorage.Fields)
		{
			if (!metadata.Fields.contains(fieldID))
			{
				SK_CORE_ERROR_TAG("Scripting", "entity storage for entity {} contains invalid field {}", entityID, fieldStorage.GetName());
				continue;
			}

			const auto& fieldMetadata = metadata.Fields.at(fieldID);

			if (fieldMetadata.DataType == ManagedFieldType::Entity)
			{
				Coral::ManagedObject value = fieldMetadata.Type->CreateInstance(fieldStorage.GetValue<uint64_t>());
				instance.SetFieldValue(fieldMetadata.Name, value);
				value.Destroy();
			}
			else if (fieldMetadata.DataType == ManagedFieldType::String)
			{
				instance.SetFieldValue<std::string>(fieldMetadata.Name, fieldStorage.GetValue<std::string>());
			}
			else
			{
				instance.SetFieldValueRaw(fieldMetadata.Name, fieldStorage.m_ValueBuffer.Data);
			}

			fieldStorage.m_Instance = &instance;
		}
		return &entityStorage.Instance;
	}

	void ScriptEngine::Destoy(UUID entityID, ScriptStorage& storage)
	{
		if (!m_CurrentScene->IsValidEntityID(entityID))
			return;

		auto& entityStorage = storage.EntityInstances.at(entityID);

		Entity entity = m_CurrentScene->GetEntityByID(entityID);
		auto& scriptComponent = entity.GetComponent<ScriptComponent>();

		entityStorage.Instance.InvokeMethod("InvokeOnDestroyed");
		scriptComponent.OnCreateCalled = false;
		scriptComponent.Instance = nullptr;
		entityStorage.Instance.Destroy();
	}

	uint64_t ScriptEngine::FindScriptMetadata(std::string_view fullName) const
	{
		uint64_t id = Hash::GenerateFNV(fullName);
		if (m_ScriptMetadata.contains(id))
			return id;

		const auto i = std::ranges::find(m_ScriptMetadata, fullName, [](auto& entry) { return entry.second.FullName; });
		if (i != m_ScriptMetadata.end())
			return i->first;

		return 0;
	}

}

