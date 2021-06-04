#pragma once

#include "Shark/Scene/NativeScript.h"

namespace Shark {

	namespace EditorData {

		struct NaticeScriptComponent
		{
			bool Found = false;
			bool Bound = false;
		};

	}

	struct NativeScriptComponent
	{
		std::string ScriptTypeName;
		NativeScript* Script = nullptr;

		NativeScript* (*CreateScript)(Entity entity);
		void (*DestroyScript)(NativeScript* ns);

		template<typename T>
		void Bind()
		{
			CreateScript = [](Entity entity) { NativeScript* s = new T(); s->m_Entity = entity; s->m_Scene = entity.GetScene(); return s; };
			DestroyScript = [](NativeScript* ns) { delete ns; ns = nullptr; };
		}

		void UnBind()
		{
			if (Script)
				DestroyScript(Script);

			CreateScript = nullptr;
			DestroyScript = nullptr;
		}
	};

}
