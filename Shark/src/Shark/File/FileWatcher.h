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
		// Note: FilePaths are relative
		struct CallbackData
		{
			FileEvent Event;
			std::filesystem::path OldFilePath;
			std::filesystem::path FilePath;
		};

	public:
		static void Init(const std::filesystem::path& directory);
		static void ShutDown();

		static void SetDirectory(const std::filesystem::path& directory);

		template<typename Func>
		static void AddCallback(const std::string& name, const Func& func) { AddCallback(name, std::function<void(const CallbackData&)>(func)); }
		static void RemoveCallback(const std::string& name);

	private:
		static void AddCallback(const std::string& name, const std::function<void(const CallbackData&)>& func);

	private:
		static void StartFileWatcher();
	};

}
