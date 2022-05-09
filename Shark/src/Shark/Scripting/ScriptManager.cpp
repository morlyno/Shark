#include "skpch.h"
#include "ScriptManager.h"

#include "Shark/Scripting/ScriptEngine.h"

#include "Shark/Scene/Components/ScriptComponent.h"

#include "Shark/Debug/Instrumentor.h"

#include <mono/metadata/appdomain.h>

namespace Shark {

	static std::unordered_map<UUID, Script> s_Scripts;

	namespace utils {

		bool SplitScriptModuleName(const std::string& scriptModuleName, std::string& out_NameSpace, std::string& out_ClassName)
		{
			SK_PROFILE_FUNCTION();

			size_t separator = scriptModuleName.rfind('.');
			if (separator == std::string::npos)
			{
				out_NameSpace = "";
				out_ClassName = scriptModuleName;
				return true;
			}

			out_NameSpace = scriptModuleName.substr(0, separator);
			out_ClassName = scriptModuleName.substr(separator + 1);
			return true;
		}

	}

	void Script::OnCreate()
	{
		SK_PROFILE_FUNCTION();

		if (m_OnCreate)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::CallMethod(m_OnCreate, object);
		}
	}

	void Script::OnDestroy()
	{
		SK_PROFILE_FUNCTION();

		if (m_OnDestroy)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::CallMethod(m_OnDestroy, object);
		}
	}

	void Script::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		if (m_OnUpdate)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::CallMethod(m_OnUpdate, object, ts);
		}
	}

	void Script::OnCollishionBegin(UUID uuid, bool isScript)
	{
		SK_PROFILE_FUNCTION();

		if (m_OnCollishionBegin)
		{
			if (isScript)
			{
				const Script& s = ScriptManager::GetScript(uuid);
				MonoObject* other = mono_gchandle_get_target(s.m_GCHandle);
				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::CallMethod(m_OnCollishionBegin, This, other);
			}
			else
			{
				MonoClass* entityClass = ScriptEngine::GetEntityClass();
				MonoObject* object = mono_object_new(mono_domain_get(), entityClass);
				GCHandle gcHandle = mono_gchandle_new(object, false);
				mono_runtime_object_init(object);

				MonoClassField* field = mono_class_get_field_from_name(entityClass, "m_Handle");
				mono_field_set_value(object, field, &uuid);

				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::CallMethod(m_OnCollishionBegin, This, object);

				mono_gchandle_free(gcHandle);
			}
		}
	}

	void Script::OnCollishionEnd(UUID uuid, bool isScript)
	{
		SK_PROFILE_FUNCTION();

		if (m_OnCollishionEnd)
		{
			if (isScript)
			{
				const Script& s = ScriptManager::GetScript(uuid);
				MonoObject* other = mono_gchandle_get_target(s.m_GCHandle);
				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::CallMethod(m_OnCollishionEnd, This, other);
			}
			else
			{
				MonoClass* entityClass = ScriptEngine::GetEntityClass();
				MonoObject* object = mono_object_new(mono_domain_get(), entityClass);
				GCHandle gcHandle = mono_gchandle_new(object, false);
				mono_runtime_object_init(object);

				MonoClassField* field = mono_class_get_field_from_name(entityClass, "m_Handle");
				mono_field_set_value(object, field, &uuid);

				MonoObject* This = mono_gchandle_get_target(m_GCHandle);
				ScriptEngine::CallMethod(m_OnCollishionEnd, This, object);
				mono_gchandle_free(gcHandle);
			}
		}
	}

	const Script& ScriptManager::Instantiate(Entity entity, bool callOnCreate)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(entity.HasComponent<ScriptComponent>());

		const auto& comp = entity.GetComponent<ScriptComponent>();
		UUID uuid = entity.GetUUID();

		std::string nameSpace;
		std::string className;
		utils::SplitScriptModuleName(comp.ScriptName, nameSpace, className);

		MonoClass* clazz = mono_class_from_name_case(ScriptEngine::GetImage(), nameSpace.c_str(), className.c_str());
		if (!clazz)
		{
			SK_CORE_WARN("Script class not Found! Namespace: {}, Class: {}", nameSpace, className);
			return s_Scripts[entity.GetUUID()];
		}

		SK_CORE_ASSERT(mono_class_is_subclass_of(clazz, ScriptEngine::GetEntityClass(), false));

		MonoObject* object = mono_object_new(mono_domain_get(), clazz);
		mono_runtime_object_init(object);
		GCHandle handle = mono_gchandle_new(object, false);

		// init Entity
		{
			MonoClass* entityClass = ScriptEngine::GetEntityClass();
			MonoClassField* field = mono_class_get_field_from_name(entityClass, "m_Handle");
			mono_field_set_value(object, field, &uuid);
		}

		Script& script = s_Scripts[uuid];
		SK_CORE_ASSERT(!script.m_GCHandle);

		script.m_GCHandle              = handle;
		script.m_OnCreate              = ScriptEngine::GetMethod(fmt::format("{}:OnCreate()", comp.ScriptName));
		script.m_OnDestroy             = ScriptEngine::GetMethod(fmt::format("{}:OnDestroy()", comp.ScriptName));
		script.m_OnUpdate              = ScriptEngine::GetMethod(fmt::format("{}:OnUpdate(Shark.TimeStep)", comp.ScriptName));
		script.m_OnCollishionBegin     = ScriptEngine::GetMethod(fmt::format("{}:OnCollishionBegin(Shark.Entity)", comp.ScriptName));
		script.m_OnCollishionEnd       = ScriptEngine::GetMethod(fmt::format("{}:OnCollishionEnd(Shark.Entity)", comp.ScriptName));

		if (callOnCreate)
			script.OnCreate();

		return script;
	}

	void ScriptManager::Destroy(Entity entity, bool callOnDestroy)
	{
		SK_PROFILE_FUNCTION();

		UUID uuid = entity.GetUUID();
		Script& script = s_Scripts.at(uuid);
		if (callOnDestroy)
			script.OnDestroy();
		mono_gchandle_free(script.m_GCHandle);

		s_Scripts.erase(uuid);
	}

	void ScriptManager::Cleanup()
	{
		SK_PROFILE_FUNCTION();

		for (auto& [uuid, script] : s_Scripts)
		{
			script.OnDestroy();
			mono_gchandle_free(script.m_GCHandle);
		}

		s_Scripts.clear();
	}

	bool ScriptManager::Contains(UUID handle)
	{
		SK_PROFILE_FUNCTION();

		return s_Scripts.find(handle) != s_Scripts.end();
	}

	Script& ScriptManager::GetScript(UUID handle)
	{
		SK_PROFILE_FUNCTION();

		return s_Scripts.at(handle);
	}

	const std::unordered_map<UUID, Script>& ScriptManager::GetScripts()
	{
		SK_PROFILE_FUNCTION();

		return s_Scripts;
	}

}
