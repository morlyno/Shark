#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"

#include "Shark/Scene/Components/ScriptComponent.h"

#include "Shark/Scripting/ScriptingGlue.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Instrumentor.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/image.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/class.h>
#include <mono/utils/mono-logger.h>
#include <mono/utils/mono-publib.h>

#define SK_MONO_LOG_LEVEL "warning"

namespace Shark {

	struct ScriptingData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* RuntimeDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreImage = nullptr;

		MonoAssembly* ScriptAssembly = nullptr;
		MonoImage* ScriptImage = nullptr;

		std::string CoreAssemblyPath;
		std::string ScriptAssemblyPath;
	};
	static ScriptingData s_ScriptData;

	static Ref<Scene> s_ActiveScene = nullptr;
	static bool s_ForceModuleUpdate = false;

	namespace utils {

		MonoClass* GetClassFromName(const std::string className, MonoImage* image)
		{
			SK_PROFILE_FUNCTION();

			size_t seperator = className.find_last_of('.');

			std::string nameSpace;
			std::string name;
			if (seperator == std::string::npos)
			{
				nameSpace = "";
				name = className;
			}
			else
			{
				nameSpace = className.substr(0, seperator);
				name = className.substr(seperator + 1);
			}

			return mono_class_from_name(image, nameSpace.c_str(), name.c_str());
		}

		Log::Level MonoLogLevelToLogLevel(std::string_view logLevel)
		{
			if (logLevel == "debug")
				return Log::Level::Debug;
			if (logLevel == "info")
				return Log::Level::Trace;
			if (logLevel == "message")
				return Log::Level::Info;
			if (logLevel == "warning")
				return Log::Level::Warn;
			if (logLevel == "critical")
				return Log::Level::Critical;
			if (logLevel == "error")
				return Log::Level::Error;
			return Log::Level::Trace;
		}

		static void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

				SK_CORE_TRACE("{0}.{1}", nameSpace, name);
			}
		}

	}

	MonoAssembly* ScriptEngine::LoadMonoAssembly(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
			return nullptr;

		Buffer fileData = FileSystem::ReadBinary(filePath);
		
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data(fileData.As<char>(), fileData.Size, true, &status);
		if (status != MONO_IMAGE_OK)
		{
			const char* errorMsg = mono_image_strerror(status);
			SK_CORE_ERROR("Failed to open Image from {0}\n\t Message: {1}", filePath, errorMsg);
			return false;
		}


		std::string assemblyName = filePath.stem().string();
		MonoAssembly* assembly = mono_assembly_load_from(image, assemblyName.c_str(), &status);
		mono_image_close(image);
		fileData.Release();
		return assembly;
	}

	void ScriptEngine::HandleException(MonoObject* exception)
	{
		if (!exception)
			return;

		MonoString* monoStr = mono_object_to_string(exception, nullptr);
		const wchar_t* str = mono_string_to_utf16(monoStr);
		SK_CONSOLE_ERROR(str);
	}

	void LogCallback(const char* log_domain, const char* log_level, const char* message, mono_bool fatal, void* user_data)
	{
		auto level = utils::MonoLogLevelToLogLevel(log_level);
		SK_CORE_LOG(level, "{}: {}{}", log_domain ? log_domain : "Unkown Domain", fatal ? "[Fatal] " : "", message);
	}

	bool ScriptEngine::Init(const std::string& scriptingCoreAssemblyPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("Init ScriptEngine [{}]", scriptingCoreAssemblyPath);

		mono_trace_set_level_string(SK_MONO_LOG_LEVEL);
		mono_trace_set_log_handler(&LogCallback, nullptr);

		s_ScriptData.CoreAssemblyPath = scriptingCoreAssemblyPath;
		mono_set_dirs(SK_CONNECT_TO_STRING(MONO_DIRECTORY, /lib), SK_CONNECT_TO_STRING(MONO_DIRECTORY, /etc));

		s_ScriptData.RootDomain = mono_jit_init("Core Domain");
		SK_CORE_ASSERT(s_ScriptData.RootDomain, "Failed to initialize Domain");

		return true;
	}

	void ScriptEngine::Shutdown()
	{
		SK_PROFILE_FUNCTION();

		if (s_ScriptData.RootDomain)
		{
			SK_CORE_INFO("ScriptEngine Shutdown");

			mono_jit_cleanup(s_ScriptData.RootDomain);
			s_ScriptData.RootDomain = nullptr;
			s_ScriptData.CoreAssembly = nullptr;
			s_ScriptData.CoreImage = nullptr;
			s_ScriptData.ScriptAssembly = nullptr;
			s_ScriptData.ScriptImage = nullptr;
			s_ScriptData.CoreAssemblyPath.clear();
			s_ScriptData.ScriptAssemblyPath.clear();
		}
	}

	bool ScriptEngine::LoadAssembly(const std::string& assemblyPath)
	{
		SK_PROFILE_FUNCTION();

		s_ScriptData.ScriptAssemblyPath = assemblyPath;

		s_ScriptData.RuntimeDomain = mono_domain_create_appdomain("Script Domain", nullptr);
		if (!s_ScriptData.RuntimeDomain)
		{
			SK_CORE_ERROR("Failed to load Runtime Domain");
			return false;
		}
		mono_domain_set(s_ScriptData.RuntimeDomain, 0);
		

		s_ScriptData.CoreAssembly = LoadMonoAssembly(s_ScriptData.CoreAssemblyPath);
		if (!s_ScriptData.CoreAssembly)
			return false;

		s_ScriptData.CoreImage = mono_assembly_get_image(s_ScriptData.CoreAssembly);


		s_ScriptData.ScriptAssembly = LoadMonoAssembly(s_ScriptData.ScriptAssemblyPath);
		if (!s_ScriptData.ScriptAssembly)
			return false;

		s_ScriptData.ScriptImage = mono_assembly_get_image(s_ScriptData.ScriptAssembly);

		//SK_CORE_INFO("Core Assembly:");
		//utils::PrintAssemblyTypes(s_ScriptData.CoreAssembly);
		
		//SK_CORE_INFO("Script Assembly:");
		//utils::PrintAssemblyTypes(s_ScriptData.ScriptAssembly);

		ScriptingGlue::Init();

		SK_CORE_INFO("ScriptEngine Assembly Loaded [{}]", s_ScriptData.ScriptAssemblyPath);

		return true;
	}

	void ScriptEngine::UnloadAssembly()
	{
		SK_PROFILE_FUNCTION();

		if (!s_ScriptData.RuntimeDomain)
			return;

		SK_CORE_INFO("ScriptEngine Assembly Unloaded [{}]", s_ScriptData.ScriptAssemblyPath);

		mono_domain_set(s_ScriptData.RootDomain, 0);
		mono_domain_unload(s_ScriptData.RuntimeDomain);
		s_ScriptData.RuntimeDomain = nullptr;
		s_ScriptData.CoreAssembly = nullptr;
		s_ScriptData.CoreImage = nullptr;
		s_ScriptData.ScriptAssembly = nullptr;
		s_ScriptData.ScriptImage = nullptr;
	}

	bool ScriptEngine::ReloadAssembly()
	{
		SK_PROFILE_FUNCTION();

		UnloadAssembly();
		return LoadAssembly(s_ScriptData.ScriptAssemblyPath);
	}

	bool ScriptEngine::AssemblyLoaded()
	{
		return s_ScriptData.ScriptAssembly;
	}

	void ScriptEngine::SetActiveScene(const Ref<Scene> scene)
	{
		s_ActiveScene = scene;
	}

	Ref<Scene> ScriptEngine::GetActiveScene()
	{
		return s_ActiveScene;
	}

	MonoImage* ScriptEngine::GetImage()
	{
		return s_ScriptData.ScriptImage;
	}

	MonoImage* ScriptEngine::GetCoreImage()
	{
		return s_ScriptData.CoreImage;
	}

	MonoClass* ScriptEngine::GetEntityClass()
	{
		return mono_class_from_name_case(s_ScriptData.CoreImage, "Shark", "Entity");
	}

	bool ScriptEngine::AssemblyHasScript(const std::string& className)
	{
		if (!s_ScriptData.ScriptAssembly)
			return false;

		return utils::GetClassFromName(className, s_ScriptData.ScriptImage) != nullptr;
	}

	MonoMethod* ScriptEngine::GetMethod(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.ScriptImage);
	}

	MonoMethod* ScriptEngine::GetMethodCore(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.CoreImage);
	}

	MonoMethod* ScriptEngine::GetClassMethod(MonoClass* clazz, const std::string& methodName, bool includeNameSpace)
	{
		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_class(methodDesc, clazz);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

	MonoObject* ScriptEngine::InvokeMethodInternal(MonoMethod* method, void* object, void** args)
	{
		MonoObject* exception = nullptr;
		MonoObject* retVal = mono_runtime_invoke(method, object, args, &exception);
		if (exception)
		{
			HandleException(exception);
			return nullptr;
		}
		return retVal;
	}

	MonoMethod* ScriptEngine::GetMethodInternal(const std::string& methodName, bool includeNameSpace, MonoImage* image)
	{
		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_image(methodDesc, image);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

}

