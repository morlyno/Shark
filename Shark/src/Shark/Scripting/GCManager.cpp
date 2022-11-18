#include "skpch.h"
#include "GCManager.h"

#include <mono\metadata\object.h>
#include <mono\metadata\mono-gc.h>

namespace Shark {

	void GCManager::Init()
	{
	}

	void GCManager::Shutdown()
	{
	}

	GCHandle GCManager::CreateHandle(MonoObject* obj)
	{
		return mono_gchandle_new_v2(obj, false);
	}

	void GCManager::ReleaseHandle(GCHandle handle)
	{
		mono_gchandle_free_v2(handle);
	}

	MonoObject* GCManager::GetManagedObject(GCHandle gcHandle)
	{
		MonoObject* object = mono_gchandle_get_target_v2(gcHandle);
		if (object && mono_object_get_vtable(object))
			return object;

		SK_CORE_WARN_TAG("Scripting", "MonoObject retrieved from GCHandle was invalid");
		return nullptr;
	}

	void GCManager::Collect()
	{
		mono_gc_collect(mono_gc_max_generation());
	}

}
