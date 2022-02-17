#pragma once

namespace Shark {

	enum class FileEvent
	{
		None = 0,
		Created,
		Deleted,
		Modified,
		OldName,
		NewName
	};

	inline std::string FileEventToString(FileEvent fileEvent)
	{
		switch (fileEvent)
		{
			case FileEvent::None: return "None";
			case FileEvent::Created: return "Created";
			case FileEvent::Deleted: return "Deleted";
			case FileEvent::Modified: return "Modified";
			case FileEvent::OldName: return "OldName";
			case FileEvent::NewName: return "NewName";
		}
		SK_CORE_ASSERT(false, "Unkown File Event");
		return "Unkown";
	}

	struct FileChangedData
	{
		FileEvent FileEvent;
		std::filesystem::path FilePath;
	};

	using FileChangedEventFn = std::function<void(const std::vector<FileChangedData>&)>;

	class FileWatcher
	{
	public:
		static void StartWatching(const std::filesystem::path& directory);
		static void StopWatching();

		static bool IsRunning();

		static void SetFileChangedCallback(FileChangedEventFn func);

	private:
		static void StartThread();
	};

}
