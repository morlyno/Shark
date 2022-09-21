#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Scene/Components.h"

#include "Shark/Scripting/ScriptGlue.h"
#include "Shark/Scripting/GCManager.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/enttDebug.h"

#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/debug-helpers.h>
#include "mono/metadata/attrdefs.h"

namespace Shark {

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* RuntimeDomain = nullptr;
		AssemblyInfo CoreAssembly;
		AssemblyInfo AppAssembly;
		bool AssembliesLoaded = false;
		bool ReloadScheduled = false;
		ScriptEngineConfig Config;
		EntityInstancesMap EntityInstances;
		Ref<Scene> ActiveScene;
		bool IsRunning = false;

		MonoClass* EntityClass = nullptr;
		MonoMethod* EntityCtor = nullptr;
		//std::unordered_map<std::string, Ref<ScriptClass>> ScriptClasses;
		ScriptClassMap ScriptClasses;

		// Entity => FieldName => Type
		std::unordered_map<UUID, FieldStorageMap> FieldStoragesMap;
	};

	struct ScriptEngineData* s_Data = nullptr;

	static void MonoTraceLogCallback(const char* log_domain, const char* log_level, const char* message, mono_bool fatal, void* user_data)
	{
		Log::Level level = Log::Level::Trace;

		if (strcmp(log_level, "debug") == 0)
			level = Log::Level::Debug;
		else if (strcmp(log_level, "info") == 0)
			level = Log::Level::Trace;
		else if (strcmp(log_level, "message") == 0)
			level = Log::Level::Info;
		else if (strcmp(log_level, "warning") == 0)
			level = Log::Level::Warn;
		else if (strcmp(log_level, "error") == 0)
			level = Log::Level::Error;
		else if (strcmp(log_level, "critical") == 0)
			level = Log::Level::Critical;

		SK_CORE_LOG(level, "{0}: {1}", log_domain ? log_domain : "Mono", message);
		SK_CORE_ASSERT(!fatal);
	}

	static void MonoPrintCallback(const char* string, mono_bool is_stdout)
	{
		if (is_stdout)
		{
			SK_CORE_TRACE("Mono: {0}", string);
			return;
		}

		std::ofstream fout{ "Logs/Mono.log" };
		fout << string;
		fout.flush();
		fout.close();
	}

	void ScriptEngine::Init(const ScriptEngineConfig& config)
	{
		s_Data = new ScriptEngineData;
		s_Data->Config = config;
		InitMono();
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		delete s_Data;
	}

	bool ScriptEngine::LoadAssemblies(const std::filesystem::path& assemblyPath)
	{
		s_Data->RuntimeDomain = mono_domain_create_appdomain("ScriptDomain", nullptr);
		SK_CORE_ASSERT(s_Data->RuntimeDomain);
		mono_domain_set(s_Data->RuntimeDomain, true);

		if (!LoadCoreAssembly(s_Data->Config.CoreAssemblyPath))
			return false;

		if (!LoadAppAssembly(assemblyPath))
			return false;

		s_Data->AssembliesLoaded = true;

		GCManager::Init();
		ScriptGlue::Init();
		ScriptUtils::Init();

		WatchingSettings settings;
		settings.Callback = [](const auto&) { ScriptEngine::ScheduleReload(); };
		settings.NofityFilter = NotifyFilter::Creation | NotifyFilter::LastWrite;
		settings.EnabledEvents = EventFilter::Created | EventFilter::Modified;
		settings.IsRecursive = false;

		Ref<FileWatcher> fileWatcher = FileSystem::GetFileWatcher();
		fileWatcher->StartWatching("AppAssembly", s_Data->AppAssembly.FilePath.parent_path(), settings);

		mono_install_unhandled_exception_hook(&ScriptEngine::UnhandledExeptionHook, nullptr);

		CacheScriptClasses();

		return true;
	}

	void ScriptEngine::ScheduleReload()
	{
		s_Data->ReloadScheduled = true;
	}

	void ScriptEngine::UnloadAssemblies()
	{
		Ref<FileWatcher> fileWatcher = FileSystem::GetFileWatcher();
		fileWatcher->StopWatching("AppAssembly");

		ScriptUtils::Shutdown();
		ScriptGlue::Shutdown();
		GCManager::Shutdown();

		s_Data->ScriptClasses.clear();
		s_Data->FieldStoragesMap.clear();

		s_Data->AssembliesLoaded = false;

		AssemblyInfo& appInfo = s_Data->AppAssembly;
		appInfo.Assembly = nullptr;
		appInfo.Image = nullptr;
		appInfo.FilePath = std::filesystem::path{};

		AssemblyInfo& coreInfo = s_Data->CoreAssembly;
		coreInfo.Assembly = nullptr;
		coreInfo.Image = nullptr;
		coreInfo.FilePath = std::filesystem::path{};

		mono_domain_set(s_Data->RootDomain, 1);
		mono_domain_unload(s_Data->RuntimeDomain);
		s_Data->RuntimeDomain = nullptr;
	}

	void ScriptEngine::Update()
	{
		if (!s_Data->IsRunning && s_Data->ReloadScheduled)
		{
			ReloadAssemblies();

			s_Data->ReloadScheduled = false;
		}
	}

	bool ScriptEngine::AssembliesLoaded()
	{
		return s_Data->AssembliesLoaded;
	}

	const AssemblyInfo& ScriptEngine::GetCoreAssemblyInfo()
	{
		return s_Data->CoreAssembly;
	}

	const AssemblyInfo& ScriptEngine::GetAppAssemblyInfo()
	{
		return s_Data->AppAssembly;
	}

	MonoDomain* ScriptEngine::GetRuntimeDomain()
	{
		return s_Data->RuntimeDomain;
	}

	bool ScriptEngine::IsRunning()
	{
		return s_Data->IsRunning;
	}

	MonoClass* ScriptEngine::GetEntityClass()
	{
		return s_Data->EntityClass;
	}

	Ref<ScriptClass> ScriptEngine::GetScriptClass(uint64_t id)
	{
		if (s_Data->ScriptClasses.find(id) != s_Data->ScriptClasses.end())
			return s_Data->ScriptClasses.at(id);
		return nullptr;
	}

	Ref<ScriptClass> ScriptEngine::GetScriptClassFromName(std::string_view fullName)
	{
		const auto i = std::find_if(s_Data->ScriptClasses.begin(), s_Data->ScriptClasses.end(), [&fullName](auto& pair) { return pair.second->GetName() == fullName; });
		if (i != s_Data->ScriptClasses.end())
			return i->second;
		return nullptr;
	}

	void ScriptEngine::InitializeFieldStorage(Ref<FieldStorage> storage, GCHandle handle)
	{
		ManagedField& field = ScriptEngine::GetFieldFromStorage(storage);
		SK_CORE_ASSERT(storage->Type == field.Type);
		if (storage->Type == ManagedFieldType::String)
		{
			storage->SetValue(field.GetValue<std::string>(handle));
			return;
		}

		if (storage->Type == ManagedFieldType::Entity)
		{
			storage->SetValue(field.GetEntity(handle));
			return;
		}

#if SK_DEBUG
		{
			MonoType* monoType = mono_field_get_type(field);
			int alignment;
			int size = mono_type_size(monoType, &alignment);
			SK_CORE_ASSERT(size <= sizeof(storage->m_Buffer));
		}
#endif

		MonoObject* obj = GCManager::GetManagedObject(handle);
		mono_field_get_value(obj, field, storage->m_Buffer);
	}

	ManagedField& ScriptEngine::GetFieldFromStorage(Ref<FieldStorage> storage)
	{
		std::string_view name = storage->Name;
		size_t i =  name.find(':');
		SK_CORE_ASSERT(i != std::string::npos, "Invalid Field name");
		std::string_view className = name.substr(0, i);
		std::string fieldName = std::string(name.substr(i + 1));
		Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(className);
		return klass->GetField(fieldName);
	}

	const ScriptClassMap& ScriptEngine::GetScriptClasses()
	{
		return s_Data->ScriptClasses;
	}

	FieldStorageMap& ScriptEngine::GetFieldStorageMap(Entity entity)
	{
		return s_Data->FieldStoragesMap[entity.GetUUID()];
	}

	void ScriptEngine::InitializeRuntime(Ref<Scene> scene)
	{
		s_Data->ActiveScene = scene;
		s_Data->IsRunning = true;

		// Destroy all entities created to recive default values
		for (auto& [uuid, handle] : s_Data->EntityInstances)
			GCManager::ReleaseHandle(handle);
		s_Data->EntityInstances.clear();
		GCManager::Collect();

		//std::unordered_map<UUID, std::unordered_map<std::string, Ref<FieldStorage>>> fieldStoragesMap;
	}

	void ScriptEngine::ShutdownRuntime()
	{
		s_Data->ActiveScene = nullptr;
		s_Data->IsRunning = false;

		for (auto& [uuid, gcHandle] : s_Data->EntityInstances)
		{
			GCManager::ReleaseHandle(gcHandle);
		}

		s_Data->EntityInstances.clear();

		GCManager::Collect();
	}

	GCHandle ScriptEngine::InstantiateEntity(Entity entity, bool invokeOnCreate, bool initializeFields)
	{
		if (!s_Data->AssembliesLoaded || !(entity && entity.AllOf<ScriptComponent>()))
			return 0;

		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
			return 0;

		ScriptComponent& scriptComp = entity.GetComponent<ScriptComponent>();
		Ref<ScriptClass> scriptClass = ScriptEngine::GetScriptClass(scriptComp.ClassID);
		SK_CORE_ASSERT(scriptClass, "Script class not set");

		MonoObject* object = InstantiateClass(scriptClass->m_Class);
		mono_runtime_object_init(object);
		InvokeMethod(object, s_Data->EntityCtor, uuid);

		GCHandle gcHandle = GCManager::CreateHandle(object);
		s_Data->EntityInstances[uuid] = gcHandle;

		if (initializeFields)
			InitializeFields(entity);

		if (invokeOnCreate)
			MethodThunks::OnCreate(gcHandle);

		return gcHandle;
	}

	void ScriptEngine::DestroyEntityInstance(Entity entity, bool invokeOnDestroy)
	{
		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) == s_Data->EntityInstances.end())
			return;

		GCHandle gcHandle = GetInstance(entity);

		if (invokeOnDestroy)
			MethodThunks::OnDestroy(gcHandle);

		GCManager::ReleaseHandle(gcHandle);
		s_Data->EntityInstances.erase(uuid);
	}

	void ScriptEngine::InitializeFields(Entity entity)
	{
		UUID uuid = entity.GetUUID();

		if (s_Data->FieldStoragesMap.find(uuid) != s_Data->FieldStoragesMap.end())
		{
			auto& scriptComp = entity.GetComponent<ScriptComponent>();
			GCHandle handle = GetInstance(entity);
			MonoObject* object = GCManager::GetManagedObject(handle);

			const FieldStorageMap& fieldStorages = s_Data->FieldStoragesMap[uuid];
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(scriptComp.ClassID);
			auto& fields = klass->m_Fields;
			for (auto& [name, field] : fields)
			{
				if (fieldStorages.find(name) == fieldStorages.end())
					continue;

				Ref<FieldStorage> storage = fieldStorages.at(name);

				if (field.Type == ManagedFieldType::Entity)
				{
					UUID entityID = storage->GetValue<UUID>();
					if (entityID == UUID::Null)
						continue;

					Entity entity = s_Data->ActiveScene->GetEntityByUUID(entityID);
					field.SetEntity(handle, entity);
					continue;
				}

				if (field.Type == ManagedFieldType::Component)
				{
					UUID entityID = storage->GetValue<UUID>();
					if (entityID == UUID::Null)
						continue;

					Entity entity = s_Data->ActiveScene->GetEntityByUUID(entityID);
					field.SetComponent(handle, entity);
					continue;
				}

				if (field.Type == ManagedFieldType::String)
				{
					field.SetValue(handle, storage->GetValue<std::string>());
					continue;
				}

				mono_field_set_value(object, field, storage->m_Buffer);
			}
		}
	}

	void ScriptEngine::OnEntityDestroyed(Entity entity)
	{
		UUID uuid = entity.GetUUID();
		if (IsInstantiated(entity))
			DestroyEntityInstance(entity, true);

		s_Data->FieldStoragesMap.erase(uuid);
	}

	void ScriptEngine::OnEntityCloned(Entity srcEntity, Entity entity)
	{
		SK_CORE_ASSERT(!IsInstantiated(entity));
		if (!IsInstantiated(srcEntity) || IsInstantiated(entity))
			return;

		MonoObject* object = GetInstanceObject(srcEntity);
		MonoObject* cloned = mono_object_clone(object);
		GCHandle handle = GCManager::CreateHandle(cloned);
		s_Data->EntityInstances[entity.GetUUID()] = handle;
	}

	bool ScriptEngine::IsInstantiated(Entity entity)
	{
		return s_Data->EntityInstances.find(entity.GetUUID()) != s_Data->EntityInstances.end();
	}

	GCHandle ScriptEngine::GetInstance(Entity entity)
	{
		UUID entityID = entity.GetUUID();
		if (s_Data->EntityInstances.find(entityID) != s_Data->EntityInstances.end())
			return s_Data->EntityInstances.at(entityID);
		return 0;
	}

	MonoObject* ScriptEngine::GetInstanceObject(Entity entity)
	{
		GCHandle handle = GetInstance(entity);
		return GCManager::GetManagedObject(handle);
	}

	MonoObject* ScriptEngine::CreateEntity(UUID uuid)
	{
		SK_CORE_ASSERT(uuid != UUID::Null);
		MonoObject* object = InstantiateClass(s_Data->EntityClass);
		mono_runtime_object_init(object);

		MonoMethodDesc* ctorDesc = mono_method_desc_new(":.ctor(ulong)", false);
		MonoMethod* ctorMethod = mono_method_desc_search_in_class(ctorDesc, s_Data->EntityClass);
		mono_method_desc_free(ctorDesc);
		InvokeMethod(object, ctorMethod, uuid);

		return object;
	}

	MonoObject* ScriptEngine::InstantiateBaseEntity(Entity entity)
	{
		UUID entityID = entity.GetUUID();
		SK_CORE_ASSERT(!IsInstantiated(entityID));
		if (IsInstantiated(entityID))
		{
			GCHandle handle = GetInstance(entityID);
			return GCManager::GetManagedObject(handle);
		}

		DEBUG_ENTITY(entity);
		SK_CORE_ASSERT(!entity.AllOf<ScriptComponent>());

		MonoObject* entityInstance = ScriptEngine::InstantiateClass(s_Data->EntityClass);
		mono_runtime_object_init(entityInstance);
		MonoMethod* ctor = mono_class_get_method_from_name(s_Data->EntityClass, ".ctor", 1);
		ScriptEngine::InvokeMethod(entityInstance, ctor, entityID);

		return entityInstance;
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* klass)
	{
		return mono_object_new(s_Data->RuntimeDomain, klass);
	}

	Ref<Scene> ScriptEngine::GetActiveScene()
	{
		return s_Data->ActiveScene;
	}

	const EntityInstancesMap& ScriptEngine::GetEntityInstances()
	{
		return s_Data->EntityInstances;
	}

	bool ScriptEngine::InvokeMethod(MonoObject* object, MonoMethod* method, void** params, MonoObject** out_RetVal)
	{
		MonoObject* exception = nullptr;
		MonoObject* retVal = mono_runtime_invoke(method, object, params, &exception);
		if (exception)
		{
			ScriptUtils::HandleException(exception);
			return false;
		}

		if (out_RetVal)
			*out_RetVal = retVal;

		return true;
	}

	MonoObject* ScriptEngine::InvokeMethodR(MonoObject* object, MonoMethod* method, void** params)
	{
		MonoObject* ret = nullptr;
		if (InvokeMethod(object, method, params, &ret))
			return ret;
		return nullptr;
	}

	bool ScriptEngine::InvokeVirtualMethod(MonoObject* object, MonoMethod* method, void** params, MonoObject** out_RetVal)
	{
		MonoMethod* virtualMethod = mono_object_get_virtual_method(object, method);
		return InvokeMethod(object, virtualMethod, params, out_RetVal);
	}

	void ScriptEngine::InitMono()
	{
		mono_trace_set_level_string("warning");
		mono_trace_set_log_handler(&MonoTraceLogCallback, nullptr);
		mono_trace_set_print_handler(&MonoPrintCallback);
		mono_trace_set_printerr_handler(&MonoPrintCallback);
		mono_set_assemblies_path("mono/lib");
		mono_install_unhandled_exception_hook(&ScriptEngine::UnhandledExeptionHook, nullptr);

		s_Data->RootDomain = mono_jit_init("RootDomain");
		SK_CORE_ASSERT(s_Data->RootDomain);
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	MonoAssembly* ScriptEngine::LoadCSAssembly(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
		{
			SK_CORE_ERROR("[ScriptEngine] Can't load Assembly! Filepath dosn't exist");
			return nullptr;
		}

		Buffer fileData = FileSystem::ReadBinary(filePath);

		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), fileData.Size, true, &status, false);
		if (status != MONO_IMAGE_OK)
		{
			const char* errorMsg = mono_image_strerror(status);
			SK_CORE_ERROR("Failed to open Image from {0}\n\t Message: {1}", filePath, errorMsg);
			return nullptr;
		}

		std::string assemblyName = filePath.stem().string();
		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyName.c_str(), &status, false);
		mono_image_close(image);
		fileData.Release();
		return assembly;
	}

	bool ScriptEngine::LoadCoreAssembly(const std::filesystem::path& filePath)
	{
		MonoAssembly* assembly = LoadCSAssembly(filePath);
		if (!assembly)
		{
			SK_CORE_ERROR("[ScriptEngine] Failed to load Core Assembly");
			return false;
		}

		AssemblyInfo& info = s_Data->CoreAssembly;
		info.Assembly = assembly;
		info.Image = mono_assembly_get_image(assembly);
		info.FilePath = filePath;
		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filePath)
	{
		MonoAssembly* assembly = LoadCSAssembly(filePath);
		if (!assembly)
		{
			SK_CORE_ERROR("[ScriptEngine] Failed to load App Assembly");
			return false;
		}

		AssemblyInfo& info = s_Data->AppAssembly;
		info.Assembly = assembly;
		info.Image = mono_assembly_get_image(assembly);
		info.FilePath = filePath;
		return true;
	}

	bool ScriptEngine::ReloadAssemblies()
	{
		SK_CORE_ASSERT(!s_Data->IsRunning, "Reloading at runntime not supported");

		// Try reload assemlies
		// is failed keep the old ones

		MonoDomain* newDomain = mono_domain_create_appdomain("ScriptDomain New", nullptr);
		SK_CORE_ASSERT(newDomain);
		mono_domain_set(newDomain, true);

		MonoAssembly* coreAssembly = LoadCSAssembly(s_Data->CoreAssembly.FilePath);
		MonoAssembly* appAssembly = LoadCSAssembly(s_Data->AppAssembly.FilePath);

		if (!coreAssembly || !appAssembly)
			return false;

		for (auto& [uuid, handle] : s_Data->EntityInstances)
			GCManager::ReleaseHandle(handle);
		s_Data->EntityInstances.clear();

		s_Data->CoreAssembly.Assembly = coreAssembly;
		s_Data->CoreAssembly.Image = mono_assembly_get_image(coreAssembly);
		s_Data->AppAssembly.Assembly = appAssembly;
		s_Data->AppAssembly.Image = mono_assembly_get_image(appAssembly);

		ScriptUtils::Shutdown();
		ScriptGlue::Shutdown();
		GCManager::Shutdown();

		mono_domain_unload(s_Data->RuntimeDomain);
		s_Data->RuntimeDomain = newDomain;

		GCManager::Init();
		ScriptGlue::Init();
		ScriptUtils::Init();

		CacheScriptClasses();

		SK_CORE_INFO("[ScriptEngine] Assemblies Reloaded");
		return true;
	}

	void ScriptEngine::CacheScriptClasses()
	{
		s_Data->EntityClass = mono_class_from_name_case(s_Data->CoreAssembly.Image, "Shark", "Entity");
		s_Data->EntityCtor = mono_class_get_method_from_name(s_Data->EntityClass, ".ctor", 1);

		MonoImage* appImage = s_Data->AppAssembly.Image;
		const MonoTableInfo* typeDefTable = mono_image_get_table_info(appImage, MONO_TABLE_TYPEDEF);
		int numTypes = mono_table_info_get_rows(typeDefTable);

		for (int i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* name = mono_metadata_string_heap(appImage, cols[MONO_TYPEDEF_NAME]);
			const char* nameSpace = mono_metadata_string_heap(appImage, cols[MONO_TYPEDEF_NAMESPACE]);
			MonoClass* klass = mono_class_from_name(appImage, nameSpace, name);
			if (!mono_class_is_subclass_of(klass, s_Data->EntityClass, false))
				continue;

			Ref<ScriptClass> scriptClass = Ref<ScriptClass>::Create(nameSpace, name);
			s_Data->ScriptClasses[scriptClass->GetID()] = scriptClass;
		}
	}

	bool ScriptEngine::IsInstantiated(UUID entityID)
	{
		return s_Data->EntityInstances.find(entityID) != s_Data->EntityInstances.end();
	}

	GCHandle ScriptEngine::GetInstance(UUID entityID)
	{
		if (s_Data->EntityInstances.find(entityID) != s_Data->EntityInstances.end())
			return s_Data->EntityInstances.at(entityID);
		return 0;
	}

	void ScriptEngine::UnhandledExeptionHook(MonoObject* exc, void* user_data)
	{
		MonoString* excecptionMessage = mono_object_to_string(exc, nullptr);
		SK_CONSOLE_ERROR(ScriptUtils::MonoStringToUTF8(excecptionMessage));
	}

}

