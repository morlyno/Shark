#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Scripting/ScriptTypes.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
}

namespace Shark {

	class GCManager
	{
	public:
		static void Init();
		static void Shutdown();

		static GCHandle CreateHandle(MonoObject* obj);
		static void ReleaseHandle(GCHandle handle);

		static MonoObject* GetManagedObject(GCHandle gcHandle);

		static void Collect();
	};

}
