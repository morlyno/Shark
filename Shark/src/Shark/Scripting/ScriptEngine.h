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

		static MonoImage* GetImage();
		static MonoImage* GetCoreImage();

		static MonoClass* GetEntityClass();

		static bool AssemblyHasScript(const std::string& className);

		static MonoMethod* GetMethod(const std::string& methodName, bool includeNameSpace = true);
		static MonoMethod* GetMethodCore(const std::string& methodName, bool includeNameSpace = true);

		static MonoMethod* GetClassMethod(MonoClass* clazz, const std::string& methodName, bool includeNameSpace = true);

	private:
		template<typename T>
		static auto ToPointer(const T* val) { return val; }
		
		template<typename T>
		static auto ToPointer(const T& val) { return &val; }
		
		template<typename T>
		static auto ToPointer(T* val) { return val; }
		
		template<typename T>
		static auto ToPointer(T& val) { return &val; }

	public:
		template<typename... Args>
		static MonoObject* CallMethod(MonoMethod* method, void* object, Args&&... arguments)
		{
			if constexpr (sizeof... (arguments) > 0)
			{
				// Note(moro): stupid hack
				const void* args[] = {
					ToPointer(arguments)...
				};
				return CallMethodInternal(method, object, (void**)args);
			}
			return CallMethodInternal(method, object, nullptr);
		}

	private:
		static MonoObject* CallMethodInternal(MonoMethod* method, void* object, void** args);
		static MonoMethod* GetMethodInternal(const std::string& methodName, bool includeNameSpace, MonoImage* image);

	private:
		static void CheckForModuleUpdate();

	};

}