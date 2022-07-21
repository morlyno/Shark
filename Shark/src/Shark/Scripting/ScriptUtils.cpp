#include "skpch.h"
#include "ScriptUtils.h"

#include "Shark/Scripting/ScriptEngine.h"

#include <mono/metadata/object.h>
#include <mono/metadata/debug-helpers.h>

namespace Shark {

	struct StackWalkerData
	{
		std::string Stack;
	};

	mono_bool StackWalker(MonoMethod* method, int32_t native_offset, int32_t il_offset, mono_bool managed, void* data)
	{
		const char* methodName = mono_method_full_name(method, true);
		StackWalkerData& swData = *(StackWalkerData*)data;
		swData.Stack += fmt::format("{0}\n", methodName);
		return true;
	}

	void ScriptUtils::HandleException(MonoObject* exception)
	{
		if (exception)
		{
			MonoString* monoStr = mono_object_to_string(exception, nullptr);
			std::string stack = WalkStack();
			SK_CONSOLE_ERROR("{0}\n{1}", MonoStringToUTF8(monoStr), stack);
		}
	}

	std::string ScriptUtils::MonoStringToUTF8(MonoString* monoStr)
	{
		char* cStr = mono_string_to_utf8(monoStr);
		std::string str = cStr;
		mono_free(cStr);
		return str;
	}

	std::string ScriptUtils::WalkStack()
	{
		StackWalkerData data;
		mono_stack_walk(&StackWalker, &data);
		return data.Stack;
	}

	const char* ScriptUtils::GetClassName(GCHandle handle)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);
		return mono_class_get_name(clazz);
	}

	void ScriptUtils::InvokeOnCreate(GCHandle handle)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);

		MonoMethodDesc* desc = mono_method_desc_new(":OnCreate()", false);
		MonoMethod* onCreate = mono_method_desc_search_in_class(desc, clazz);
		if (onCreate)
			ScriptEngine::InvokeMethod(object, onCreate);
	}

	void ScriptUtils::InvokeOnDestroy(GCHandle handle)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);

		MonoMethodDesc* desc = mono_method_desc_new(":OnDestroy()", false);
		MonoMethod* onDestroy = mono_method_desc_search_in_class(desc, clazz);
		if (onDestroy)
			ScriptEngine::InvokeMethod(object, onDestroy);
	}

	void ScriptUtils::InvokeOnUpdate(GCHandle handle, TimeStep ts)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);

		MonoMethodDesc* desc = mono_method_desc_new(":OnUpdate(TimeStep)", false);
		MonoMethod* method = mono_method_desc_search_in_class(desc, clazz);
		if (method)
			ScriptEngine::InvokeMethod(object, method, ts);
	}

	void ScriptUtils::InvokeOnPhysicsUpdate(GCHandle handle, TimeStep ts)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);

		MonoMethodDesc* desc = mono_method_desc_new(":OnPhysicsUpdate(TimeStep)", false);
		MonoMethod* method = mono_method_desc_search_in_class(desc, clazz);
		if (method)
			ScriptEngine::InvokeMethod(object, method, ts);
	}

	void ScriptUtils::InvokeOnUIRender(GCHandle handle)
	{
		MonoObject* object = mono_gchandle_get_target(handle);
		MonoClass* clazz = mono_object_get_class(object);

		MonoMethodDesc* desc = mono_method_desc_new(":OnUIRender()", false);
		MonoMethod* method = mono_method_desc_search_in_class(desc, clazz);
		if (method)
			ScriptEngine::InvokeMethod(object, method);
	}

	bool ScriptUtils::ValidScriptName(const std::string& fullName)
	{
		if (!ScriptEngine::AssembliesLoaded())
			return false;

		size_t i = fullName.rfind('.');

		std::string nameSpace = fullName.substr(0, i);
		std::string name = fullName.substr(i + 1);

		MonoClass* clazz = mono_class_from_name_case(ScriptEngine::GetAppAssemblyInfo().Image, nameSpace.c_str(), name.c_str());
		return clazz != nullptr;
	}

}
