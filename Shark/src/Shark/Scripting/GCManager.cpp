#include "skpch.h"
#include "GCManager.h"

#include <mono\metadata\object.h>

namespace Shark {

	MonoObject* GCManager::GetManagedObject(GCHandle gcHandle)
	{
		return mono_gchandle_get_target(gcHandle);
	}

}
