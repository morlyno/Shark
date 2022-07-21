#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Scene/Components/ScriptComponent.h"

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

	enum
	{
		CoreAssemblyIndex = 0,
		AppAssemblyIndex = 1,
		AssemblyCount = 2
	};

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* RuntimeDomain = nullptr;
		AssemblyInfo Assemblies[AssemblyCount];
		bool AssembliesLoaded = false;
		ScriptEngineConfig Config;
		EntityInstancesMap EntityInstances;
		Ref<Scene> ActiveScene;
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

		ScriptGlue::Init();

		mono_install_unhandled_exception_hook(&ScriptEngine::UnhandledExeptionHook, nullptr);

		return true;
	}

	void ScriptEngine::ReloadAssemblies(const std::filesystem::path& assemblyPath)
	{
		UnloadAssemblies();
		LoadAssemblies(assemblyPath);
	}

	void ScriptEngine::UnloadAssemblies()
	{
		s_Data->AssembliesLoaded = false;

		AssemblyInfo& appInfo = s_Data->Assemblies[AppAssemblyIndex];
		appInfo.Assembly = nullptr;
		appInfo.Image = nullptr;
		appInfo.FilePath = std::filesystem::path{};

		AssemblyInfo& coreInfo = s_Data->Assemblies[CoreAssemblyIndex];
		coreInfo.Assembly = nullptr;
		coreInfo.Image = nullptr;
		coreInfo.FilePath = std::filesystem::path{};

		mono_domain_set(s_Data->RootDomain, 1);
		mono_domain_unload(s_Data->RuntimeDomain);
		s_Data->RuntimeDomain = nullptr;
	}

	bool ScriptEngine::AssembliesLoaded()
	{
		return s_Data->AssembliesLoaded;
	}

	const AssemblyInfo& ScriptEngine::GetCoreAssemblyInfo()
	{
		return s_Data->Assemblies[CoreAssemblyIndex];
	}

	const AssemblyInfo& ScriptEngine::GetAppAssemblyInfo()
	{
		return s_Data->Assemblies[AppAssemblyIndex];
	}

	void ScriptEngine::ShutdownRuntime()
	{
		for (auto& [uuid, gcHandle] : s_Data->EntityInstances)
			mono_gchandle_free(gcHandle);

		s_Data->EntityInstances.clear();

		GCManager::Collect();
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
		
		auto [nameSpace, name] = utils::SliptClassFullName(scriptComp.ScriptName);
		MonoClass* clazz = mono_class_from_name_case(s_Data->Assemblies[AppAssemblyIndex].Image, nameSpace.c_str(), name.c_str());
		if (!clazz)
			return false;

		MonoClass* entityClass = mono_class_from_name_case(s_Data->Assemblies[CoreAssemblyIndex].Image, "Shark", "Entity");
		mono_class_is_subclass_of(entityClass, clazz, false);

		MonoObject* object = mono_object_new(s_Data->RuntimeDomain, clazz);
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

		GCHandle gcHandle = mono_gchandle_new(object, false);
		s_Data->EntityInstances[uuid] = gcHandle;

		if (invokeOnCreate)
			ScriptUtils::InvokeOnCreate(gcHandle);

		return true;
	}

	void ScriptEngine::DestroyEntity(Entity entity, bool invokeOnDestroy)
	{
		UUID uuid = entity.GetUUID();
		if (s_Data->EntityInstances.find(uuid) == s_Data->EntityInstances.end())
		{
			SK_CORE_ERROR("[ScriptEngine] Call DestroyEntity but entity instance dosn't exist");
			return;
		}

		GCHandle gcHandle = GetEntityInstance(entity.GetUUID());
		MonoObject* object = mono_gchandle_get_target(gcHandle);
		MonoClass* clazz = mono_object_get_class(object);

		if (invokeOnDestroy)
			ScriptUtils::InvokeOnDestroy(gcHandle);

		mono_gchandle_free(gcHandle);
		s_Data->EntityInstances.erase(uuid);
	}

	GCHandle ScriptEngine::CreateTempEntity(Entity entity)
	{
		// TODO(moro): maby cache object for later use

		MonoClass* entityClass = mono_class_from_name_case(s_Data->Assemblies[CoreAssemblyIndex].Image, "Shark", "Entity");
		MonoObject* object = mono_object_new(s_Data->RuntimeDomain, entityClass);
		GCHandle persistentHandle = mono_gchandle_new(object, false);
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

		mono_gchandle_free(handle);
	}

	bool ScriptEngine::ContainsEntityInstance(UUID uuid)
	{
		return s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end();
	}

	GCHandle ScriptEngine::GetEntityInstance(UUID uuid)
	{
		return s_Data->EntityInstances.at(uuid);
	}

	const EntityInstancesMap& ScriptEngine::GetEntityInstances()
	{
		return s_Data->EntityInstances;
	}

	void ScriptEngine::SetActiveScene(const Ref<Scene>& scene)
	{
		s_Data->ActiveScene = scene;
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
		std::string path = PlatformUtils::GetEnvironmentVariable("MONO_PATH") + "/lib";
		mono_set_assemblies_path(path.c_str());
		mono_install_unhandled_exception_hook(&ScriptEngine::UnhandledExeptionHook, nullptr);

		s_Data->RootDomain = mono_jit_init("RootDomain");
		SK_CORE_ASSERT(s_Data->RootDomain);
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	bool ScriptEngine::LoadCoreAssembly(const std::filesystem::path& filePath)
	{
		MonoAssembly* assembly = LoadMonoAssembly(filePath);
		if (!assembly)
		{
			SK_CORE_ERROR("[ScriptEngine] Failed to load Core Assembly");
			return false;
		}

		AssemblyInfo& info = s_Data->Assemblies[CoreAssemblyIndex];
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

		AssemblyInfo& info = s_Data->Assemblies[AppAssemblyIndex];
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

	void ScriptEngine::UnhandledExeptionHook(MonoObject* exc, void* user_data)
	{
		MonoString* excecptionMessage = mono_object_to_string(exc, nullptr);
		SK_CONSOLE_ERROR(ScriptUtils::MonoStringToUTF8(excecptionMessage));
	}

}

