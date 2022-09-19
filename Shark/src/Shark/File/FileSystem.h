#pragma once

#include "Shark/Core/Base.h"
#include "Shark/File/FileWatcher.h"

#include "Shark/Core/Buffer.h"

namespace Shark {

	class FileSystem
	{
	public:
		static void Init();
		static void Shutdown();
		static void ProcessEvents();

		static void StartWatching(const std::filesystem::path& dirPath);
		static void StopWatching();
		static void SetCallback(FileWatcherCallbackFunc callback);
		static Ref<FileWatcher> GetFileWatcher();


		static std::filesystem::path MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName);
		static void MakeFreeFilePath(std::filesystem::path& fsPath);

		static bool CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName);

		static Buffer ReadBinary(const std::filesystem::path& filePath);
		static void TruncateFile(const std::filesystem::path& filePath);

	};

}
