#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"
#include "Shark/File/FileWatcher.h"

#include "Shark/Core/Buffer.h"

#undef CreateFile

namespace Shark {

	class FileSystem
	{
	public:
		static constexpr std::string_view InvalidCharacters = "\\/:*?\"<>|";
		static constexpr std::wstring_view InvalidCharactersW = L"\\/:*?\"<>|";

	public:
		static void Initialize();
		static void Shutdown();
		static void ProcessEvents();

		static void StartWatching(const std::filesystem::path& dirPath);
		static void StopWatching();
		static void SetCallback(FileWatcherCallbackFunc callback);
		static Ref<FileWatcher> GetFileWatcher();

		static std::filesystem::path GetUnsusedPath(const std::filesystem::path& filePath);
		static std::filesystem::path MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName);
		static void MakeFreeFilePath(std::filesystem::path& fsPath);

		static bool IsValidFileName(std::string_view fileName);

		static bool CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName);

		static bool CreateFile(const std::filesystem::path& filePath, bool overrideExisiting = false);

		static Buffer ReadBinary(const std::filesystem::path& filePath);
		static std::string ReadString(const std::filesystem::path& filePath);
		static bool WriteBinary(const std::filesystem::path& filePath, Buffer fileData, bool createDirectoriesIfNeeded = true);
		static bool WriteString(const std::filesystem::path& filePath, const std::string& fileData, bool createDirectoriesIfNeeded = true);

		static void TruncateFile(const std::filesystem::path& filePath);

	public: // custom filesystem (works for filepath relative to project)
		static bool Exists(const std::filesystem::path& filepath);
		static std::string ParseFileName(const std::filesystem::path& filePath);

		static std::filesystem::path GetRelative(const std::filesystem::path& path);
		static std::filesystem::path GetAbsolute(const std::filesystem::path& path);
	};

}
