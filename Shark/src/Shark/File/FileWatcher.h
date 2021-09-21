#pragma once

namespace Shark {

	class FileWatcher
	{
	public:
		FileWatcher();
		FileWatcher(FileWatcher&&) = default;
		FileWatcher& operator=(FileWatcher&&) = default;
		FileWatcher(const std::filesystem::path& directoryPath, bool watchSubTrees);
		~FileWatcher();

		void SetDirectory(const std::filesystem::path& directoryPath) { m_Directory = directoryPath; }
		void WatchSubTrees(bool watchSubTrees) { m_WatchSubTrees = watchSubTrees; }

		const std::filesystem::path& GetWatchingDirectory() const { return m_Directory; }

		void Start();
		void Stop();

		std::function<void(const std::filesystem::path& FilePath, const std::filesystem::path& OldFilePath)> OnRename;
		std::function<void(const std::filesystem::path& FilePath)> OnChanged;
		std::function<void(const std::filesystem::path& FilePath)> OnCreated;
		std::function<void(const std::filesystem::path& FilePath)> OnDeleted;

	private:
		void StartThread();

	private:
		std::thread m_Thread;
		bool m_Running = false;
		std::filesystem::path m_Directory;
		bool m_WatchSubTrees = false;

#ifdef SK_PLATFORM_WINDOWS
		HANDLE m_Win32_StopEvent = NULL;
#endif

	};

}
