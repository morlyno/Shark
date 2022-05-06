#include "skpch.h"
#include "Shark/File/FileWatcher.h"

#include "Platform/Windows/WindowsFileWatcher.h"

namespace Shark {

#if SK_PLATFORM_WINDOWS
	Ref<FileWatcher> FileWatcher::Create(const FileWatcherSpecification& specs)
	{
		return Ref<WindowsFileWatcher>::Create(specs);
	}
#endif

}

