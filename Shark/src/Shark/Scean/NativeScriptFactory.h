#pragma once

#include "Shark/Scean/NativeScript.h"
#include "Shark/Scean/Components/NativeScriptComponent.h"

#define SK_REGISTER_SCRIPT(script_type) ::Shark::Empty impl_##script_type##_inst = ::Shark::NativeScriptFactory::Add<script_type>(SK_STRINGIFY(script_type))
#define SK_SCRIPT_CLASS(script_name) class script_name; SK_REGISTER_SCRIPT(script_name); class script_name : public ::Shark::NativeScript

namespace Shark {

	class NativeScriptFactory
	{
	public:
		template<typename T>
		static NativeScriptFactory& Add(const std::string& scriptname)
		{
			Get().m_ScriptMap[scriptname] = [](auto& comp) { comp.Bind<T>(); };
			return Get();
		}

		static bool Exist(const std::string& scriptname)
		{
			return Get().m_ScriptMap.find(scriptname) != Get().m_ScriptMap.end();
		}

		static void Bind(const std::string& scriptname, NativeScriptComponent& comp)
		{
			SK_CORE_ASSERT(Exist(scriptname), "Script Dosen't Exitst");
			Get().m_ScriptMap[scriptname](comp);
		}

		static const auto& GetMap() { return Get().m_ScriptMap; }
	private:
		static NativeScriptFactory& Get()
		{
			static NativeScriptFactory s_Inst;
			return s_Inst;
		}
	private:
		std::unordered_map<std::string, void(*)(NativeScriptComponent&)> m_ScriptMap;
	};

}