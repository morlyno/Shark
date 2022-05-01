#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Scene/Entity.h"
#include <mono/metadata/object-forward.h>

namespace Shark {

	class ScriptEngine
	{
	public:
		static bool Init(const std::string& scriptinCoreAssemblyPath);
		static void Shutdown();

		static bool LoadAssembly(const std::string& assemblyPath);
		static void UnloadAssembly();
		static bool ReloadAssembly();

		static bool AssemblyLoaded();

		static void SetActiveScene(const Ref<Scene> scene);
		static Ref<Scene> GetActiveScene();

		static bool HasScriptClass(const std::string& className);

		static bool InstantiateEntity(Entity entity);
		static bool CreateScript(const std::string& scriptName, UUID entityUUID);
		static bool CreateScript(MonoClass* scriptClass, UUID entityUUID);
		static void DestroyScript(UUID handle);
		static void UpdateScript(UUID handle, TimeStep ts);


		static bool IsValidScriptUUID(UUID handle);

		static MonoObject* GetScriptObject(UUID handle);

		static MonoMethod* GetMethod(const std::string& methodName, bool includeNameSpace = true);
		static MonoMethod* GetMethodCore(const std::string& methodName, bool includeNameSpace = true);

		static MonoObject* CallMethod(MonoMethod* method, void* object, void** args);

	private:
		static MonoMethod* GetMethodInternal(const std::string methodName, bool includeNameSpace, MonoImage* image);

	private:
		static void CheckForModuleUpdate();

	};

}
