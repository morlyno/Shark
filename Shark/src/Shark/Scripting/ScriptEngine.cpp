#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Scene/Components.h"

#include "Shark/Scripting/ScriptGlue.h"
#include "Shark/Scripting/GCManager.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Profiler.h"

#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>

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

		const char* domain = log_domain ? log_domain : "Mono";
		Log::LogMessage(Log::Logger::Core, level, "", "[{0}] {1}", domain, message);
		SK_CORE_ASSERT(!fatal);
	}

	static void MonoPrintCallback(const char* string, mono_bool is_stdout)
	{
		if (is_stdout)
		{
			SK_CORE_TRACE_TAG("Mono", string);
			return;
		}

		std::ofstream fout{ "Logs/Mono.log" };
		fout << string;
		fout.flush();
		fout.close();
	}

	void ScriptEngine::Init(const ScriptEngineConfig& config)
	{
		SK_CORE_INFO_TAG("Scripting", "Script Engine is Initializing");

		s_Data = sknew ScriptEngineData;
		s_Data->Config = config;
		InitMono();
	}

	void ScriptEngine::Shutdown()
	{
		SK_CORE_INFO_TAG("Scripting", "Script Engine is Shutting down");

		if (s_Data->AssembliesLoaded)
			UnloadAssemblies();

		ShutdownMono();
		skdelete s_Data;
	}

	bool ScriptEngine::LoadAssemblies(const std::filesystem::path& assemblyPath)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Scripting", "Loading Assemblies");

		if (!s_Data)
		{
			SK_CORE_WARN("Core", "ScriptEngine.LoadAssemblies called but ScriptEngine is not Initialized!");
			return false;
		}

		WatchingSettings settings;
		settings.Callback = [](const auto&) { ScriptEngine::ScheduleReload(); };
		settings.NofityFilter = NotifyFilter::Creation | NotifyFilter::LastWrite;
		settings.EnabledEvents = EventFilter::Created | EventFilter::Modified;
		settings.IsRecursive = false;

		Ref<FileWatcher> fileWatcher = FileSystem::GetFileWatcher();
		fileWatcher->StartWatching("AppAssembly", assemblyPath.parent_path(), settings);


		s_Data->RuntimeDomain = mono_domain_create_appdomain("ScriptDomain", nullptr);
		SK_CORE_VERIFY(s_Data->RuntimeDomain);
		mono_domain_set(s_Data->RuntimeDomain, true);

		s_Data->CoreAssembly.FilePath = s_Data->Config.CoreAssemblyPath;
		s_Data->AppAssembly.FilePath = assemblyPath;

		if (!LoadCoreAssembly(s_Data->Config.CoreAssemblyPath))
			return false;

		if (!LoadAppAssembly(assemblyPath))
			return false;

		s_Data->AssembliesLoaded = true;

		GCManager::Init();
		ScriptGlue::Init();
		ScriptUtils::Init();

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
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Scripting", "Unloading Assemblies");

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
		SK_PROFILE_FUNCTION();
		ManagedField& field = ScriptEngine::GetFieldFromStorage(storage);
		SK_CORE_VERIFY(storage->Type == field.Type);
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
		SK_PROFILE_FUNCTION();

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
		SK_PROFILE_FUNCTION();

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
		SK_PROFILE_FUNCTION();
		if (!s_Data->AssembliesLoaded || !(entity && entity.AllOf<ScriptComponent>()))
			return 0;

		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
			return 0;

		ScriptComponent& scriptComp = entity.GetComponent<ScriptComponent>();
		Ref<ScriptClass> scriptClass = ScriptEngine::GetScriptClass(scriptComp.ClassID);
		SK_CORE_VERIFY(scriptClass, "Script class not set");

		MonoObject* object = InstantiateClass(scriptClass->m_Class);
		mono_runtime_object_init(object);
		InvokeMethod(object, s_Data->EntityCtor, uuid);

		GCHandle gcHandle = GCManager::CreateHandle(object);
		s_Data->EntityInstances[uuid] = gcHandle;

		// NOTE(moro): this is probably no longer necassary becaus entity fields get a new allocated object
		//             the actual instance is provided when calling Entity.As
		if (initializeFields)
			InitializeFields(entity);

		if (invokeOnCreate)
			MethodThunks::OnCreate(gcHandle);

		return gcHandle;
	}

	void ScriptEngine::DestroyEntityInstance(Entity entity, bool invokeOnDestroy)
	{
		SK_PROFILE_FUNCTION();
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
		SK_PROFILE_FUNCTION();
		if (!s_Data->AssembliesLoaded)
			return;

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
		SK_PROFILE_FUNCTION();
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
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(uuid != UUID::Null);
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
		SK_PROFILE_FUNCTION();
		UUID entityID = entity.GetUUID();
		SK_CORE_ASSERT(!IsInstantiated(entityID));
		if (IsInstantiated(entityID))
		{
			GCHandle handle = GetInstance(entityID);
			return GCManager::GetManagedObject(handle);
		}

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
		SK_PROFILE_FUNCTION();
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
		SK_PROFILE_FUNCTION();
		MonoObject* ret = nullptr;
		if (InvokeMethod(object, method, params, &ret))
			return ret;
		return nullptr;
	}

	bool ScriptEngine::InvokeVirtualMethod(MonoObject* object, MonoMethod* method, void** params, MonoObject** out_RetVal)
	{
		SK_PROFILE_FUNCTION();
		MonoMethod* virtualMethod = mono_object_get_virtual_method(object, method);
		return InvokeMethod(object, virtualMethod, params, out_RetVal);
	}

	void ScriptEngine::InitMono()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Scripting", "Initializing Mono");

		FileSystem::TruncateFile("Logs/Mono.log");
		
		mono_trace_set_level_string("warning");
		mono_trace_set_log_handler(&MonoTraceLogCallback, nullptr);
		mono_trace_set_print_handler(&MonoPrintCallback);
		mono_trace_set_printerr_handler(&MonoPrintCallback);
		mono_set_assemblies_path("mono/lib");
		mono_install_unhandled_exception_hook(&ScriptEngine::UnhandledExeptionHook, nullptr);

		if (s_Data->Config.EnableDebugging)
		{
			const char* argv[2] = {
				"--debugger-agent=transport=dt_socket,address=127.0.0.1:2550,server=y,suspend=n,loglevel=3,logfile=Logs/MonoDebugger.log",
				"--soft-breakpoints"
			};

			mono_jit_parse_options(2, (char**)argv);
			mono_debug_init(MONO_DEBUG_FORMAT_MONO);
		}

		s_Data->RootDomain = mono_jit_init("RootDomain");
		SK_CORE_VERIFY(s_Data->RootDomain);

		if (s_Data->Config.EnableDebugging)
			mono_debug_domain_create(s_Data->RootDomain);

		mono_thread_set_main(mono_thread_current());
	}

	void ScriptEngine::ShutdownMono()
	{
		SK_PROFILE_FUNCTION();
		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	MonoAssembly* ScriptEngine::LoadCSAssembly(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();
		if (!std::filesystem::exists(filePath))
		{
			SK_CORE_ERROR_TAG("Scripting", "Can't load Assembly! Filepath dosn't exist");
			return nullptr;
		}

		Buffer fileData = FileSystem::ReadBinary(filePath);

		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), (uint32_t)fileData.Size, true, &status, false);
		if (status != MONO_IMAGE_OK)
		{
			const char* errorMsg = mono_image_strerror(status);
			SK_CORE_ERROR_TAG("Scripting", "Failed to open Image from {0}\n\t Message: {1}", filePath, errorMsg);
			return nullptr;
		}

		if (s_Data->Config.EnableDebugging)
		{
			std::filesystem::path pdbPath = filePath;
			pdbPath.replace_extension(".pdb");

			if (FileSystem::Exists(pdbPath))
			{
				Buffer pdbData = FileSystem::ReadBinary(pdbPath);
				mono_debug_open_image_from_memory(image, pdbData.As<mono_byte>(), (int)pdbData.Size);
				SK_CORE_INFO_TAG("Scripting", "Loaded PDB {}", pdbPath);
				pdbData.Release();
			}
		}

		std::string assemblyName = filePath.stem().string();
		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyName.c_str(), &status, false);
		mono_image_close(image);
		fileData.Release();

		SK_CORE_INFO_TAG("Scripting", "Loaded Assembly: {}", filePath);
		return assembly;
	}

	bool ScriptEngine::LoadCoreAssembly(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();
		MonoAssembly* assembly = LoadCSAssembly(filePath);
		if (!assembly)
		{
			SK_CORE_ERROR_TAG("Scripting", "Failed to load Core Assembly");
			return false;
		}

		AssemblyInfo& info = s_Data->CoreAssembly;
		info.Assembly = assembly;
		info.Image = mono_assembly_get_image(assembly);
		info.FilePath = filePath;

		SK_CORE_INFO_TAG("Scripting", "Core Assembly Loaded. ({0})", info.FilePath);
		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();
		MonoAssembly* assembly = LoadCSAssembly(filePath);
		if (!assembly)
		{
			SK_CORE_ERROR_TAG("Scripting", "Failed to load App Assembly");
			return false;
		}

		AssemblyInfo& info = s_Data->AppAssembly;
		info.Assembly = assembly;
		info.Image = mono_assembly_get_image(assembly);
		info.FilePath = filePath;

		SK_CORE_INFO_TAG("Scripting", "App Assembly Loaded. ({0})", info.FilePath);
		return true;
	}

	bool ScriptEngine::ReloadAssemblies()
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(!s_Data->IsRunning, "Reloading at runntime not supported");

		SK_CORE_INFO_TAG("Scripting", "Reloading Assemblies");

		// Try reload assemlies
		// if failed keep the old ones

		MonoDomain* newDomain = mono_domain_create_appdomain("ScriptDomain", nullptr);
		SK_CORE_VERIFY(newDomain);
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

		SK_CONSOLE_INFO("Assemblies Reloaded");
		SK_CORE_INFO_TAG("Scripting", "Assemblies Reloaded");
		return true;
	}

	void ScriptEngine::CacheScriptClasses()
	{
		SK_PROFILE_FUNCTION();
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

