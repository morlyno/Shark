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

	void Script::OnCreate() const
	{
		if (m_OnCreate)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::InvokeMethod(m_OnCreate, object);
		}
	}

	void Script::OnDestroy() const
	{
		if (m_OnDestroy)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::InvokeMethod(m_OnDestroy, object);
		}
	}

	void Script::OnUpdate(TimeStep ts) const
	{
		if (m_OnUpdate)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::InvokeMethod(m_OnUpdate, object, ts);
		}
	}

	void Script::OnPhysicsUpdate(TimeStep fixedTimeStep) const
	{
		if (m_OnPhyicsUpdate)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::InvokeMethod(m_OnPhyicsUpdate, object, fixedTimeStep);
		}
	}

	void Script::OnUIRender() const
	{
		if (m_OnUIRender)
		{
			MonoObject* object = mono_gchandle_get_target(m_GCHandle);
			ScriptEngine::InvokeMethod(m_OnUIRender, object);
		}
	}

	void Script::OnCollishionBegin(UUID uuid, bool isScript) const
	{
		if (m_OnCollishionBegin)
		{
			if (isScript)
			{
				const Script& s = ScriptManager::GetScript(uuid);
				MonoObject* other = mono_gchandle_get_target(s.m_GCHandle);
				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::InvokeMethod(m_OnCollishionBegin, This, other);
			}
			else
			{
				MonoClass* entityClass = ScriptEngine::GetEntityClass();
				MonoObject* object = mono_object_new(mono_domain_get(), entityClass);
				GCHandle gcHandle = mono_gchandle_new(object, false);
				mono_runtime_object_init(object);

				MonoProperty* prop = mono_class_get_property_from_name(entityClass, "ID");
				void* params[] = { &uuid };
				mono_property_set_value(prop, object, params, nullptr);

				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::InvokeMethod(m_OnCollishionBegin, This, object);

				mono_gchandle_free(gcHandle);
			}
		}
	}

	void Script::OnCollishionEnd(UUID uuid, bool isScript) const
	{
		if (m_OnCollishionEnd)
		{
			if (isScript)
			{
				const Script& s = ScriptManager::GetScript(uuid);
				MonoObject* other = mono_gchandle_get_target(s.m_GCHandle);
				MonoObject* This = mono_gchandle_get_target(m_GCHandle);

				ScriptEngine::InvokeMethod(m_OnCollishionEnd, This, other);
			}
			else
			{
				MonoClass* entityClass = ScriptEngine::GetEntityClass();
				MonoObject* object = mono_object_new(mono_domain_get(), entityClass);
				GCHandle gcHandle = mono_gchandle_new(object, false);
				mono_runtime_object_init(object);

				MonoProperty* prop = mono_class_get_property_from_name(entityClass, "ID");
				void* params[] = { &uuid };
				mono_property_set_value(prop, object, params, nullptr);

				MonoObject* This = mono_gchandle_get_target(m_GCHandle);
				ScriptEngine::InvokeMethod(m_OnCollishionEnd, This, object);
				mono_gchandle_free(gcHandle);
			}
		}
	}

	bool ScriptManager::Instantiate(Entity entity, bool callOnCreate)
	{
		SK_PROFILE_FUNCTION();

		if (!ScriptEngine::GetImage())
		{
			SK_CORE_ERROR("No Script loaded");
			return false;
		}

		SK_CORE_ASSERT(entity.AllOf<ScriptComponent>());

		const auto& comp = entity.GetComponent<ScriptComponent>();
		SK_CORE_ASSERT(!comp.HasRuntime, "Called ScriptManager::Instantiate on allready instantiated Script");
		UUID uuid = entity.GetUUID();

		std::string nameSpace;
		std::string className;
		utils::SplitScriptModuleName(comp.ScriptName, nameSpace, className);

		MonoClass* clazz = mono_class_from_name_case(ScriptEngine::GetImage(), nameSpace.c_str(), className.c_str());
		if (!clazz)
		{
			SK_CORE_WARN("Script class not Found! Namespace: {}, Class: {}", nameSpace, className);
			return false;
		}

		SK_CORE_ASSERT(mono_class_is_subclass_of(clazz, ScriptEngine::GetEntityClass(), false));

		MonoObject* object = mono_object_new(mono_domain_get(), clazz);
		mono_runtime_object_init(object);
		GCHandle handle = mono_gchandle_new(object, false);

		// init Entity
		MonoClass* entityClass = ScriptEngine::GetEntityClass();
		{
			MonoProperty* prop = mono_class_get_property_from_name(entityClass, "ID");
			void* params[] = {
				&uuid
			};
			mono_property_set_value(prop, object, params, nullptr);
		}

		Script& script = s_Scripts[uuid];
		SK_CORE_ASSERT(!script.m_GCHandle);

		script.m_GCHandle              = handle;
		script.m_OnCreate              = ScriptEngine::GetMethod(fmt::format("{}:OnCreate()", comp.ScriptName));
		script.m_OnDestroy             = ScriptEngine::GetMethod(fmt::format("{}:OnDestroy()", comp.ScriptName));
		script.m_OnUpdate              = ScriptEngine::GetMethod(fmt::format("{}:OnUpdate(Shark.TimeStep)", comp.ScriptName));
		script.m_OnPhyicsUpdate        = ScriptEngine::GetMethod(fmt::format("{}:OnPhysicsUpdate(Shark.TimeStep)", comp.ScriptName));
		script.m_OnUIRender            = ScriptEngine::GetMethod(fmt::format("{}:OnUIRender()", comp.ScriptName));
		script.m_OnCollishionBegin     = ScriptEngine::GetMethod(fmt::format("{}:OnCollishionBegin(Shark.Entity)", comp.ScriptName));
		script.m_OnCollishionEnd       = ScriptEngine::GetMethod(fmt::format("{}:OnCollishionEnd(Shark.Entity)", comp.ScriptName));

		if (callOnCreate)
			script.OnCreate();

		return true;
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
		return s_Scripts.find(handle) != s_Scripts.end();
	}

	Script& ScriptManager::GetScript(UUID handle)
	{
		return s_Scripts.at(handle);
	}

	const std::unordered_map<UUID, Script>& ScriptManager::GetScripts()
	{
		return s_Scripts;
	}

}
