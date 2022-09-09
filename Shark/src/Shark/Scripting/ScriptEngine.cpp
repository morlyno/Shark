#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Scene/Components.h"

#include "Shark/Scripting/ScriptGlue.h"
#include "Shark/Scripting/GCManager.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"

#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/debug-helpers.h>

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
		//std::unordered_map<std::string, Ref<ScriptClass>> ScriptClasses;
		std::unordered_map<uint64_t, Ref<ScriptClass>> ScriptClasses;

		// Entity => FieldName => Type
		std::unordered_map<UUID, FieldStorageMap> FieldStoragesMap;
	};

	struct ScriptEngineData* s_Data = nullptr;

	namespace utils {

		static std::pair<std::string, std::string> SliptClassFullName(const std::string& fullClassName)
		{
			size_t s = fullClassName.rfind('.');
			if (s == std::string::npos)
				return {};

			return {
				fullClassName.substr(0, s),
				fullClassName.substr(s + 1)
			};
		}

	}

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

	MonoClass* ScriptEngine::GetEntityClass()
	{
		return s_Data->EntityClass;
	}

	Ref<ScriptClass> ScriptEngine::GetScriptClassFromName(const std::string& fullName)
	{
		const auto i = std::find_if(s_Data->ScriptClasses.begin(), s_Data->ScriptClasses.end(), [&fullName](auto& pair) { return pair.second->GetName() == fullName; });
		if (i != s_Data->ScriptClasses.end())
			return i->second;
		return nullptr;

		//if (s_Data->ScriptClasses.find(fullName) != s_Data->ScriptClasses.end())
		//	return s_Data->ScriptClasses.at(fullName);
		//return nullptr;
	}

	Ref<ScriptClass> ScriptEngine::GetScriptClass(uint64_t id)
	{
		if (s_Data->ScriptClasses.find(id) != s_Data->ScriptClasses.end())
			return s_Data->ScriptClasses.at(id);
		return nullptr;
	}

	FieldStorageMap& ScriptEngine::GetFieldStorageMap(Entity entity)
	{
		return s_Data->FieldStoragesMap[entity.GetUUID()];
	}

	void ScriptEngine::InitializeFieldStorage(Ref<FieldStorage> storage, GCHandle handle)
	{
		MonoObject* obj = GCManager::GetManagedObject(handle);
#if SK_DEBUG
		{
			MonoType* monoType = mono_field_get_type(storage->Field);
			int alignment;
			int size = mono_type_size(monoType, &alignment);
			SK_CORE_ASSERT(size <= sizeof(storage->m_Buffer));
		}
#endif

		mono_field_get_value(obj, storage->Field, storage->m_Buffer);
	}

	void ScriptEngine::OnRuntimeStart(Ref<Scene> scene)
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

	void ScriptEngine::OnRuntimeShutdown()
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

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* klass)
	{
		return mono_object_new(s_Data->RuntimeDomain, klass);
	}

	bool ScriptEngine::InstantiateEntity(Entity entity, bool invokeOnCreate)
	{
		if (!s_Data->AssembliesLoaded || !(entity && entity.AllOf<ScriptComponent>()))
			return false;

		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
		{
			SK_CORE_ERROR("[ScriptEngine] Called InstantiateEntity twice on same entity");
			return false;
		}

		ScriptComponent& scriptComp = entity.GetComponent<ScriptComponent>();
		Ref<ScriptClass> scriptClass = ScriptEngine::GetScriptClass(scriptComp.ClassID);
		SK_CORE_ASSERT(scriptClass, "Script class not set");
		MonoClass* clazz = scriptClass->m_Class;
		MonoClass* entityClass = s_Data->EntityClass;

		//auto [nameSpace, name] = utils::SliptClassFullName(scriptComp.ScriptName);
		//MonoClass* clazz = mono_class_from_name_case(s_Data->AppAssembly.Image, nameSpace.c_str(), name.c_str());
		//if (!clazz)
		//	return false;
		//
		//MonoClass* entityClass = mono_class_from_name_case(s_Data->CoreAssembly.Image, "Shark", "Entity");
		//SK_CORE_ASSERT(mono_class_is_subclass_of(clazz, entityClass, false));

		MonoObject* object = InstantiateClass(clazz);
		mono_runtime_object_init(object);
		{
			MonoMethodDesc* methodDesc = mono_method_desc_new(":.ctor(ulong)", false);
			MonoMethod* method = mono_method_desc_search_in_class(methodDesc, entityClass);
			mono_method_desc_free(methodDesc);
			MonoObject* exception = nullptr;
			void* params[] = { &uuid };
			mono_runtime_invoke(method, object, params, &exception);
			ScriptUtils::HandleException(exception);
		}

		//SK_CORE_ASSERT(s_Data->FieldStorages.find(uuid) != s_Data->FieldStorages.end(), fmt::format("No Field Storages for Entity ({})", entity.GetName()));
		//SK_CORE_ASSERT(scriptComp.ScriptName != "Sandbox.PlayerController");
		if (s_Data->FieldStoragesMap.find(uuid) != s_Data->FieldStoragesMap.end())
		{
			const FieldStorageMap& fieldStorages = s_Data->FieldStoragesMap[uuid];
			Ref<ScriptClass> klass = ScriptEngine::GetScriptClass(scriptComp.ClassID);
			auto& fields = klass->m_Fields;
			for (const auto& [name, fieldStorage] : fieldStorages)
				mono_field_set_value(object, fields[name].Field, fieldStorage->m_Buffer);
		}

		GCHandle gcHandle = GCManager::CreateHandle(object);
		s_Data->EntityInstances[uuid] = gcHandle;

		if (invokeOnCreate)
			MethodThunks::OnCreate(gcHandle);

		return true;
	}

	void ScriptEngine::DestroyInstance(Entity entity, bool invokeOnDestroy)
	{
		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) == s_Data->EntityInstances.end())
		{
			SK_CORE_ERROR("[ScriptEngine] Call DestroyEntity but entity instance dosn't exist");
			return;
		}

		GCHandle gcHandle = GetInstance(entity);

		if (invokeOnDestroy)
			MethodThunks::OnDestroy(gcHandle);

		GCManager::ReleaseHandle(gcHandle);
		s_Data->EntityInstances.erase(uuid);
	}

	void ScriptEngine::OnEntityDestroyed(Entity entity)
	{
		UUID uuid = entity.GetUUID();
		if (IsInstantiated(entity))
			DestroyInstance(entity, true);

		s_Data->FieldStoragesMap.erase(uuid);
	}

	GCHandle ScriptEngine::CreateTempEntity(Entity entity)
	{
		// TODO(moro): maby cache object for later use

		MonoClass* entityClass = mono_class_from_name_case(s_Data->CoreAssembly.Image, "Shark", "Entity");
		MonoObject* object = InstantiateClass(entityClass);
		GCHandle persistentHandle = GCManager::CreateHandle(object);
		mono_runtime_object_init(object);

		MonoMethodDesc* ctorDesc = mono_method_desc_new(":.ctor(ulong)", false);
		MonoMethod* ctorMethod = mono_method_desc_search_in_class(ctorDesc, entityClass);
		mono_method_desc_free(ctorDesc);
		InvokeMethod(object, ctorMethod, entity.GetUUID());

		return persistentHandle;
	}

	void ScriptEngine::ReleaseTempEntity(GCHandle handle)
	{
		// TODO(moro): maby cache object for later use

		GCManager::ReleaseHandle(handle);
	}
	bool ScriptEngine::IsInstantiated(Entity entity)
	{
		return s_Data->EntityInstances.find(entity.GetUUID()) != s_Data->EntityInstances.end();
	}

	GCHandle ScriptEngine::GetInstance(Entity entity)
	{
		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
			return s_Data->EntityInstances.at(uuid);
		return 0;
	}

	const EntityInstancesMap& ScriptEngine::GetEntityInstances()
	{
		return s_Data->EntityInstances;
	}

	Ref<Scene> ScriptEngine::GetActiveScene()
	{
		return s_Data->ActiveScene;
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

	void ScriptEngine::ReloadAssemblies()
	{
		SK_CORE_ASSERT(!s_Data->IsRunning, "Reloading at runntime not supported");

		for (auto& [uuid, handle] : s_Data->EntityInstances)
			GCManager::ReleaseHandle(handle);
		s_Data->EntityInstances.clear();

		auto fieldStorageBackup = s_Data->FieldStoragesMap;
		std::filesystem::path assemblyPath = s_Data->AppAssembly.FilePath;
		UnloadAssemblies();
		LoadAssemblies(assemblyPath);
		
		s_Data->FieldStoragesMap = fieldStorageBackup;
	}

	bool ScriptEngine::LoadCoreAssembly(const std::filesystem::path& filePath)
	{
		MonoAssembly* assembly = LoadMonoAssembly(filePath);
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
		MonoAssembly* assembly = LoadMonoAssembly(filePath);
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

	MonoAssembly* ScriptEngine::LoadMonoAssembly(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
			return nullptr;

		Buffer fileData = FileSystem::ReadBinary(filePath);

		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), fileData.Size, true, &status, false);
		if (status != MONO_IMAGE_OK)
		{
			const char* errorMsg = mono_image_strerror(status);
			SK_CORE_ERROR("Failed to open Image from {0}\n\t Message: {1}", filePath, errorMsg);
			return false;
		}

		std::string assemblyName = filePath.stem().string();
		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyName.c_str(), &status, false);
		mono_image_close(image);
		fileData.Release();
		return assembly;
	}

	void ScriptEngine::CacheScriptClasses()
	{
		s_Data->EntityClass = mono_class_from_name_case(s_Data->CoreAssembly.Image, "Shark", "Entity");

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

	void ScriptEngine::UnhandledExeptionHook(MonoObject* exc, void* user_data)
	{
		MonoString* excecptionMessage = mono_object_to_string(exc, nullptr);
		SK_CONSOLE_ERROR(ScriptUtils::MonoStringToUTF8(excecptionMessage));
	}

}

