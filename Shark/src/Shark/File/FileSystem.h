#pragma once

#include "Shark/Core/Base.h"
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


		static std::filesystem::path MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName);
		static void MakeFreeFilePath(std::filesystem::path& fsPath);

		static bool CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName);

		static Buffer ReadBinary(const std::filesystem::path& filePath);

	};

}
