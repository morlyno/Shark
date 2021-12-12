#pragma once

namespace Shark {

	enum class FileEvent
	{
		None = 0,
		Created,
		Deleted,
		Modified,
		Renamed
	};

	inline std::string FileEventToString(FileEvent fileEvent)
	{
		switch (fileEvent)
		{
			case FileEvent::None: return "None";
			case FileEvent::Created: return "Created";
			case FileEvent::Deleted: return "Deleted";
			case FileEvent::Modified: return "Modified";
			case FileEvent::Renamed: return "Renamed";
		}
		SK_CORE_ASSERT(false, "Unkown File Event");
		return "Unkown";
	}

	class FileWatcher
	{
	public:
		static void StartWatching(const std::filesystem::path& directory);
		static void StopWatching();

		static bool IsRunning();

	private:
		static void StartThread();
	};

}
