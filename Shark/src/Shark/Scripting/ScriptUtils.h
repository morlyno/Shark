#pragma once

#include "Shark/Core/TimeStep.h"

#include "Shark/Scripting/ScriptTypes.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoString MonoString;
	typedef struct _MonoMethod MonoMethod;
}

namespace Shark {

	class ScriptUtils
	{
	public:
		static void HandleException(MonoObject* exception);
		static char* MonoStringToUTF8(MonoString* monoStr);

		static std::string WalkStack();

		static const char* GetClassName(GCHandle handle);

		static void InvokeOnCreate(GCHandle handle);
		static void InvokeOnDestroy(GCHandle handle);
		static void InvokeOnUpdate(GCHandle handle, TimeStep ts);
		static void InvokeOnPhysicsUpdate(GCHandle handle, TimeStep ts);
		static void InvokeOnUIRender(GCHandle handle);

		static bool ValidScriptName(const std::string& fullName);
	};

}
