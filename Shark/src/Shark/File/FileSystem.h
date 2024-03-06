#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"
#include "Shark/Core/Buffer.h"


#undef CreateFile
#undef CopyFile


namespace Shark {

	class FileSystem
	{
	public:
		static constexpr std::string_view InvalidCharacters = "\\/:*?\"<>|";
		static constexpr std::wstring_view InvalidCharactersW = L"\\/:*?\"<>|";

	public:
		static void Initialize();
		static void Shutdown();

	public:
		static Buffer ReadBinary(const std::filesystem::path& filePath);
		static std::string ReadString(const std::filesystem::path& filePath);
		static bool WriteBinary(const std::filesystem::path& filePath, Buffer fileData, bool createDirectoriesIfNeeded = true);
		static bool WriteString(const std::filesystem::path& filePath, const std::string& fileData, bool createDirectoriesIfNeeded = true);

	public:
		static bool IsValidFilename(std::string_view filename);

	public:
		static bool Exists(const std::filesystem::path& filepath);
		static bool Exists(const std::filesystem::path& filepath, std::string& errorMsg);

		static std::filesystem::path WorkingDirectory();
		static std::filesystem::path Absolute(const std::filesystem::path& filepath);
		static std::filesystem::path Relative(const std::filesystem::path& filepath, const std::filesystem::path& base = WorkingDirectory());

		static bool CreateFile(const std::filesystem::path& filepath, bool overrideExisiting = false);
		static bool CreateFile(const std::filesystem::path& filepath, bool overrideExisiting, std::string& errorMsg);

		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::string& errorMsg);
		static bool CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options, std::string& errorMsg);

		static bool CreateDirectories(const std::filesystem::path& path);
		static bool CreateDirectories(const std::filesystem::path& path, std::string& errorMsg);

		static bool Rename(const std::filesystem::path& oldName, const std::string& newName);
		static bool Rename(const std::filesystem::path& oldName, const std::string& newName, std::string& errorMsg);

		static bool Move(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
		static bool Move(const std::filesystem::path& oldPath, const std::filesystem::path& newPath, std::string& errorMsg);

		static bool Remove(const std::filesystem::path& path);
		static bool Remove(const std::filesystem::path& path, std::string& errorMsg);
		static uint64_t RemoveAll(const std::filesystem::path& path);
		static uint64_t RemoveAll(const std::filesystem::path& path, std::string& errorMsg);

		static void TruncateFile(const std::filesystem::path& filepath);
		static bool IsInDirectory(const std::filesystem::path& directory, const std::filesystem::path& path);
		static std::filesystem::path GetFilesystemPath(const std::filesystem::path& path);

		static void RemoveExtension(std::filesystem::path& path);
		static void ReplaceExtension(std::filesystem::path& path, const std::string& extension);
		static void ReplaceStem(std::filesystem::path& path, const std::string& stem);
		static void ReplaceFilename(std::filesystem::path& path, const std::string& filename);

		static std::filesystem::path ChangeExtension(const std::filesystem::path& path, const std::string& extesnion);
		static std::filesystem::path ChangeFilename(const std::filesystem::path& path, const std::string& filename);

		static std::string GetStemString(const std::filesystem::path& path);
		static std::string GetExtensionString(const std::filesystem::path& path);

		static std::filesystem::path CreatePath(const std::filesystem::path& directory, const std::string& name, const std::string& extension);
		static std::string CreatePathString(const std::filesystem::path& directory, const std::string& name, const std::string& extension);
	};

}
