#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"

#include "Shark/Core/Buffer.h"

#include <filewatch/FileWatch.hpp>

#undef CreateFile
#undef CopyFile

namespace Shark {

	struct FileEvent
	{
		filewatch::Event Type;
		std::filesystem::path File;
	};

	using FileWatchCallbackFn = std::function<void(const std::vector<FileEvent>&)>;

	class FileSystem
	{
		// Custom Filesystem API for Filesystem and Project Path
		// 
		// This engine uses 3 different types of path
		// 
		// 1. Filesystem:
		//    An absolute path
		// 
		// 2. Project:
		//    Relative to the Project directory.
		//    Made absolute by calling Project::GetAbsolute on the active project
		// 
		// 3. Asset:
		//    Relative to the Assets directory.
		//    Used by the ResourceManager.
		//    This type dosen't work with the custom FileSystem API
		// 
		// !!! Important !!!
		// A path relative to the Working Directory can not be hanled by the Sharks Filesystem API.
		// If path relative to hte Working Directory is necessary it must be converted into a Filesystem Path by calling std::filesystem::absolute
		// 
		// If a function is called whit an absolute path the API hanles it as a Filesystem Path.
		// Otherwithe the path is handled as a Project Path and make absolute by calling Project::GetAbsolute.
		// The api can't distinguish between a relative path 
		//

	public:
		static constexpr std::string_view InvalidCharacters = "\\/:*?\"<>|";
		static constexpr std::wstring_view InvalidCharactersW = L"\\/:*?\"<>|";

	public:
		static void Initialize();
		static void Shutdown();

	public:
		static void ProcessEvents();
		static void StartWatch(const std::filesystem::path& watchPath, FileWatchCallbackFn callback);
		static void StartWatch(const std::filesystem::path& watchPath, std::wregex regex, FileWatchCallbackFn callback);
		static void StopWatch(const std::filesystem::path& watchPath);

	public:
		static Buffer ReadBinary(const std::filesystem::path& filePath);
		static std::string ReadString(const std::filesystem::path& filePath);
		static bool WriteBinary(const std::filesystem::path& filePath, Buffer fileData, bool createDirectoriesIfNeeded = true);
		static bool WriteString(const std::filesystem::path& filePath, const std::string& fileData, bool createDirectoriesIfNeeded = true);

	public:
		static bool IsValidFileName(std::string_view fileName);
		static bool CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName);

		static bool IsInDirectory(const std::filesystem::path& directory, const std::filesystem::path& path);

	public:
		static bool Exists(const std::filesystem::path& filepath);
		static bool Exists(const std::filesystem::path& filepath, std::string& errorMsg);

		static bool CreateFile(const std::filesystem::path& filepath, bool overrideExisiting = false);
		static bool CreateFile(const std::filesystem::path& filepath, bool overrideExisiting, std::string& errorMsg);

		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::string& errorMsg);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options, std::string& errorMsg);

		static bool CreateDirectories(const std::filesystem::path& path);
		static bool CreateDirectories(const std::filesystem::path& path, std::string& errorMsg);

		static void TruncateFile(const std::filesystem::path& filepath);

	public:
		static std::filesystem::path GetFilesystemPath(const std::filesystem::path& projectOrFilesystemPath);

		// Takes a path relative to the working directory and returns an absolute path
		// Used for Editor Resource with are usually relative to the Working Directory
		static std::filesystem::path GetResourcePath(const std::filesystem::path& filepath);
	};

	std::string ToString(filewatch::Event event);

	std::filesystem::path operator""_abs(const char* str, size_t length);
	std::filesystem::path operator""_abs(const wchar_t* str, size_t length);

}

template<>
struct fmt::formatter<Shark::FileEvent>
{
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const Shark::FileEvent& data, FormatContext& ctx) -> decltype(ctx.out())
	{
		format_to(ctx.out(), "({0}) {1}", Shark::ToString(data.Type), data.File);
		return ctx.out();
	}

};