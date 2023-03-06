#include "skpch.h"
#include "ScriptUtils.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/GCManager.h"

#include "Shark/Debug/Profiler.h"

#include <mono/metadata/object.h>
#include <mono/metadata/debug-helpers.h>

namespace Shark {

	struct ScriptUtilsData
	{
		UnmanagedThunk<MonoObject*> OnCreate;
		UnmanagedThunk<MonoObject*> OnDestroy;
		UnmanagedThunk<MonoObject*, float> OnUpdate;
		UnmanagedThunk<MonoObject*, float> OnPhyiscsUpdate;
		UnmanagedThunk<MonoObject*> OnUIRender;
	};
	static ScriptUtilsData* s_UtilsData = nullptr;

	void ScriptUtils::Init()
	{
		s_UtilsData = new ScriptUtilsData();
		MonoClass* entityClass = mono_class_from_name_case(ScriptEngine::GetCoreAssemblyInfo().Image, "Shark", "Entity");
		s_UtilsData->OnCreate = mono_class_get_method_from_name(entityClass, "OnCreate", 0);
		s_UtilsData->OnDestroy = mono_class_get_method_from_name(entityClass, "OnDestroy", 0);
		s_UtilsData->OnUpdate = mono_class_get_method_from_name(entityClass, "OnUpdate", 1);
		s_UtilsData->OnPhyiscsUpdate = mono_class_get_method_from_name(entityClass, "OnPhysicsUpdate", 1);
		s_UtilsData->OnUIRender = mono_class_get_method_from_name(entityClass, "OnUIRender", 0);
	}

	void ScriptUtils::Shutdown()
	{
		delete s_UtilsData;
		s_UtilsData = nullptr;
	}

	void ScriptUtils::HandleException(MonoObject* exception)
	{
		if (exception)
		{
			std::string msg = ObjectToString(exception);
			SK_CONSOLE_ERROR(msg);
		}
	}

	bool ScriptUtils::CheckMonoError(MonoError& error)
	{
		if (!mono_error_ok(&error))
		{
			const char* msg = mono_error_get_message(&error);
			SK_CORE_ERROR_TAG("Scripting", msg);
			mono_error_cleanup(&error);
			return true;
		}
		return false;
	}

	std::string ScriptUtils::MonoStringToUTF8(MonoString* monoStr)
	{
		MonoError error;
		char* cStr = mono_string_to_utf8_checked(monoStr, &error);
		if (ScriptUtils::CheckMonoError(error))
			return std::string{};

		std::string str = cStr;
		mono_free(cStr);
		return str;
	}

	MonoString* ScriptUtils::UTF8ToMonoString(const std::string& str)
	{
		return mono_string_new_len(ScriptEngine::GetRuntimeDomain(), str.c_str(), (uint32_t)str.length());
	}

	MonoString* ScriptUtils::MonoStringEmpty()
	{
		return mono_string_empty(ScriptEngine::GetRuntimeDomain());
	}

	std::string ScriptUtils::ObjectToString(MonoObject* obj)
	{
		MonoString* monoStr = mono_object_to_string(obj, nullptr);
		return MonoStringToUTF8(monoStr);
	}

	UUID ScriptUtils::GetIDFromEntity(MonoObject* object)
	{
		MonoClass* klass = mono_object_get_class(object);
		MonoClassField* idField = mono_class_get_field_from_name(klass, "ID");
		SK_CORE_ASSERT(idField, "Field Is not an entity");
		uint64_t id = 0;
		mono_field_get_value(object, idField, &id);
		return id;
	}

	MonoObject* ScriptUtils::GetOrCreateEntity(Entity entity)
	{
		if (ScriptEngine::IsInstantiated(entity))
			return ScriptEngine::GetInstanceObject(entity);
		return ScriptEngine::CreateEntity(entity.GetUUID());
	}

	const char* ScriptUtils::GetClassName(GCHandle handle)
	{
		MonoObject* object = GCManager::GetManagedObject(handle);
		MonoClass* clazz = mono_object_get_class(object);
		return mono_class_get_name(clazz);
	}

	std::string_view ScriptUtils::GetFieldName(const ManagedField& field)
	{
		return mono_field_get_name(field);
	}

	void* ScriptUtils::GetUnmanagedThunk(MonoMethod* method)
	{
		return mono_method_get_unmanaged_thunk(method);
	}

	void MethodThunks::OnCreate(GCHandle handle)
	{
		SK_PROFILE_FUNCTION();
		MonoException* exception = nullptr;
		MonoObject* object = GCManager::GetManagedObject(handle);
		s_UtilsData->OnCreate.Invoke(object, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnDestroy(GCHandle handle)
	{
		SK_PROFILE_FUNCTION();
		MonoException* exception = nullptr;
		MonoObject* object = GCManager::GetManagedObject(handle);
		s_UtilsData->OnDestroy.Invoke(object, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnUpdate(GCHandle handle, float ts)
	{
		SK_PROFILE_FUNCTION();
		MonoException* exception = nullptr;
		MonoObject* object = GCManager::GetManagedObject(handle);
		s_UtilsData->OnUpdate.Invoke(object, ts, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnPhysicsUpdate(GCHandle handle, float ts)
	{
		SK_PROFILE_FUNCTION();
		MonoException* exception = nullptr;
		MonoObject* object = GCManager::GetManagedObject(handle);
		s_UtilsData->OnPhyiscsUpdate.Invoke(object, ts, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void MethodThunks::OnUIRender(GCHandle handle)
	{
		SK_PROFILE_FUNCTION();
		MonoException* exception = nullptr;
		MonoObject* object = GCManager::GetManagedObject(handle);
		s_UtilsData->OnUIRender.Invoke(object, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

}
