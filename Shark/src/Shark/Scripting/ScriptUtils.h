#pragma once

#include "Shark/Core/TimeStep.h"

#include "Shark/Scripting/ScriptTypes.h"

#undef GetClassName

extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoString MonoString;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoException MonoException;
	typedef union _MonoError MonoErrorExternal;
	typedef MonoErrorExternal MonoError;
}

namespace Shark {

	class ScriptUtils;

	template<typename... TParams>
	struct UnmanagedThunk
	{
		using Thunk = void(*)(TParams..., MonoException**);
		Thunk Method = nullptr;

		UnmanagedThunk() = default;
		UnmanagedThunk(Thunk thunk)
			: Method(thunk)
		{}

		UnmanagedThunk(MonoMethod* method)
		{
			SetThunkFromMethod(method);
		}

		void Invoke(TParams... params, MonoException** exception)
		{
			Method(params..., exception);
		}

		void SetThunkFromMethod(MonoMethod* method)
		{
			Method = (Thunk)ScriptUtils::GetUnmanagedThunk(method);
		}

	};

	template<typename TReturn, typename... TParams>
	struct UnmanagedThunkR
	{
		using Thunk = void(*)(TParams..., MonoException**);
		Thunk Method = nullptr;

		UnmanagedThunkR() = default;
		UnmanagedThunkR(Thunk thunk)
			: Method(thunk)
		{}

		UnmanagedThunkR(MonoMethod* method)
		{
			SetThunkFromMethod(method);
		}

		TReturn Invoke(TParams... params, MonoException** exception)
		{
			return Method(params..., exception);
		}

		void SetThunkFromMethod(MonoMethod* method)
		{
			Method = (Thunk)ScriptUtils::GetUnmanagedThunk(method);
		}

	};

	class ScriptUtils
	{
	public:
		static void Init();
		static void Shutdown();

		static void HandleException(MonoObject* exception);
		static bool CheckMonoError(MonoError& error);

		// string
		static std::string MonoStringToUTF8(MonoString* monoStr);
		static MonoString* UTF8ToMonoString(const std::string& str);
		static MonoString* MonoStringEmpty();
		static std::string ObjectToString(MonoObject* obj);

		static const char* GetClassName(GCHandle handle);

	private:
		static void* GetUnmanagedThunk(MonoMethod* method);

		template<typename...>
		friend struct UnmanagedThunk;
		template<typename, typename...>
		friend struct UnmanagedThunkR;
	};

	struct MethodThunks
	{
		static void OnCreate(GCHandle handle);
		static void OnDestroy(GCHandle handle);
		static void OnUpdate(GCHandle handle, float ts);
		static void OnPhysicsUpdate(GCHandle handle, float ts);
		static void OnUIRender(GCHandle handle);
	};

}
