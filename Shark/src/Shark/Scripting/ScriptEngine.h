#pragma once

#include "Shark/Scene/Scene.h"

namespace Shark {

	class ScriptEngine
	{
	public:
		static bool Init(const std::string& scriptinCoreAssemblyPath);
		static void Shutdown();

		static bool LoadAssembly(const std::string& assemblyPath);
		static void UnloadAssembly();
		static bool ReloadAssembly();

	private:
		static bool CopyScriptModule();

	};

}
