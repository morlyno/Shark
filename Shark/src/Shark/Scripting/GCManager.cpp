#include "skpch.h"
#include "GCManager.h"

#include <mono\metadata\object.h>
#include <mono\metadata\mono-gc.h>

namespace Shark {

	struct GCManagerData
	{
		std::unordered_set<GCHandle> Handles;
	};
	static GCManagerData* s_GCData = nullptr;

	void GCManager::Init()
	{
		s_GCData = new GCManagerData();
	}

	void GCManager::Shutdown()
	{
		delete s_GCData;
		s_GCData = nullptr;
	}

	GCHandle GCManager::CreateHandle(MonoObject* obj)
	{
		GCHandle handle = mono_gchandle_new(obj, false);
		SK_CORE_ASSERT(s_GCData->Handles.find(handle) == s_GCData->Handles.end());
		s_GCData->Handles.emplace(handle);
		return handle;
	}

	void GCManager::ReleaseHandle(GCHandle handle)
	{
		SK_CORE_ASSERT(s_GCData->Handles.find(handle) != s_GCData->Handles.end());
		s_GCData->Handles.erase(handle);
		mono_gchandle_free(handle);
	}

	MonoObject* GCManager::GetManagedObject(GCHandle gcHandle)
	{
		SK_CORE_ASSERT(s_GCData->Handles.find(gcHandle) != s_GCData->Handles.end());
		return mono_gchandle_get_target(gcHandle);
	}

	void GCManager::Collect()
	{
		mono_gc_collect(mono_gc_max_generation());
	}

}
