#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Scripting/ScriptTypes.h"
#include "Shark/Scripting/ScriptUtils.h"

#include "Shark/utils/Utils.h"

extern "C" {
	typedef struct _MonoDomain MonoDomain;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
}

namespace Shark {

	using EntityInstancesMap = std::unordered_map<UUID, GCHandle>;
	using ScriptClassMap = std::unordered_map<uint64_t, Ref<ScriptClass>>;
	using FieldStorageMap = std::map<std::string, Ref<FieldStorage>>;

	struct ScriptEngineConfig
	{
		std::filesystem::path CoreAssemblyPath;
	};

	class ScriptEngine
	{
	public:
		static void Init(const ScriptEngineConfig& config);
		static void Shutdown();

		static bool LoadAssemblies(const std::filesystem::path& assemblyPath);
		static void ScheduleReload();
		static void UnloadAssemblies();
		static void Update();

		static bool AssembliesLoaded();

		static const AssemblyInfo& GetCoreAssemblyInfo();
		static const AssemblyInfo& GetAppAssemblyInfo();

		static bool IsRunning();

		static MonoDomain* GetRuntimeDomain();

		static MonoClass* GetEntityClass();
		static const ScriptClassMap& GetScriptClasses();
		static Ref<ScriptClass> GetScriptClassFromName(std::string_view fullName);
		static Ref<ScriptClass> GetScriptClass(uint64_t id);

		static FieldStorageMap& GetFieldStorageMap(Entity entity);
		static void InitializeFieldStorage(Ref<FieldStorage> storage, GCHandle handle);
		static ManagedField& GetFieldFromStorage(Ref<FieldStorage> storage);

	public: // Scripting API
		static void InitializeRuntime(Ref<Scene> scene);
		static void ShutdownRuntime();

		static MonoObject* InstantiateClass(MonoClass* klass);

		static GCHandle InstantiateBaseEntity(Entity entity);
		static GCHandle InstantiateEntity(Entity entity, bool invokeOnCreate, bool initializeFields);
		static void DestroyInstance(Entity entity, bool invokeOnDestroy);

		static void InitializeFields(Entity entity);

		static void OnEntityDestroyed(Entity entity);

		static bool IsInstantiated(Entity entity);
		static GCHandle GetInstance(Entity entity);

		static const EntityInstancesMap& GetEntityInstances();
		static Ref<Scene> GetActiveScene();

	public:
		template<typename... TArgs>
		static bool InvokeMethod(MonoObject* object, MonoMethod* method, TArgs&&... args)
		{
			void* params[] = { (void*)&args... };
			return InvokeMethod(object, method, params);
		}

		template<typename... TArgs>
		static bool InvokeVirtualMethod(MonoObject* object, MonoMethod* method, TArgs&&... args)
		{
			void* params[] = { (void*)&args... };
			return InvokeVirtualMethod(object, method, params);
		}

		static bool InvokeMethod(MonoObject* object, MonoMethod* method, void** params = nullptr, MonoObject** out_RetVal = nullptr);
		static MonoObject* InvokeMethodR(MonoObject* object, MonoMethod* method, void** params = nullptr);
		static bool InvokeVirtualMethod(MonoObject* object, MonoMethod* method, void** params = nullptr, MonoObject** out_RetVal = nullptr);

	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoAssembly* LoadCSAssembly(const std::filesystem::path& filePath);

		static bool LoadCoreAssembly(const std::filesystem::path& filePath);
		static bool LoadAppAssembly(const std::filesystem::path& filePath);

		static bool ReloadAssemblies();

		static MonoObject* CreateEntity(UUID uuid);
		static void CacheScriptClasses();

		static bool IsInstantiated(UUID entityID);
		static GCHandle GetInstance(UUID entityID);

		static void UnhandledExeptionHook(MonoObject* exc, void* user_data);
	};

}
