#pragma once

#include "Shark/Scean/NativeScript.h"

namespace Shark {

	struct NativeScriptComponent
	{
		NativeScript* Script;

		NativeScript* (*CreateScript)(Entity entity);
		void (*DestroyScript)(NativeScript* ns);

		template<typename Type>
		void Bind()
		{
			CreateScript = [](Entity entity) { NativeScript* s = reinterpret_cast<NativeScript*>(new Type()); s->SetEntity(entity); return s; };
			DestroyScript = [](NativeScript* ns) { delete ns; ns = nullptr; };
		}
	};

}
