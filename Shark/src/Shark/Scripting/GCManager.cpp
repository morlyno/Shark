#include "skpch.h"
#include "GCManager.h"

#include <mono\metadata\object.h>
#include <mono\metadata\mono-gc.h>

namespace Shark {

	MonoObject* GCManager::GetManagedObject(GCHandle gcHandle)
	{
		return mono_gchandle_get_target(gcHandle);
	}

	void GCManager::Collect()
	{
		mono_gc_collect(mono_gc_max_generation());
	}

}
