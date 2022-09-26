#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"
#include "Shark/File/FileWatcher.h"

#include "Shark/Core/Buffer.h"

namespace Shark {

	class FileSystem
	{
	public:
		static void StartWatching();
		static void StartWatching(const std::filesystem::path& directory);
		static void StartWatching(const std::filesystem::path& directory, FileWatcherCallbackFunc callback);
		static void StartWatching(const FileWatcherSpecification& specs);
		static void StopWatching();

		static void PauseWatching();
		static void ContinueWatching();
		static void SkipNextFileEvent();

		static void SetFileWatcherCallback(FileWatcherCallbackFunc callback);

		static Ref<FileWatcher> GetFileWatcher();

		static std::filesystem::path GetUnsusedPath(const std::filesystem::path& filePath);
		static std::filesystem::path MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName);
		static void MakeFreeFilePath(std::filesystem::path& fsPath);

		static bool CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName);

		static Buffer ReadBinary(const std::filesystem::path& filePath);
		static void TruncateFile(const std::filesystem::path& filePath);

	public: // custom filesystem (works for filepath relative to project)
		static bool Exists(const std::filesystem::path& filepath);
		static std::string ParseFileName(const std::filesystem::path& filePath);

		static std::filesystem::path GetRelative(const std::filesystem::path& path);
		static std::filesystem::path GetAbsolute(const std::filesystem::path& path);
	};

}
