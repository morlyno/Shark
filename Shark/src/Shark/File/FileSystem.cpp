#include "skpch.h"
#include "FileSystem.h"

#include "Shark/Core/Project.h"
#include "Shark/Utils/String.h"
#include "Shark/Utils/PlatformUtils.h"

#include <fmt/os.h>

namespace Shark {

#define SK_FILESYSTEM_THROW 0
#if SK_FILESYSTEM_THROW
#define SK_HANDLE_FS_ERROR(_function, ...) throw std::filesystem::filesystem_error(_function, __VA_ARGS__)
#else
#define SK_HANDLE_FS_ERROR(...) SK_CORE_VERIFY(false);
#endif

	void FileSystem::Initialize()
	{
	}

	void FileSystem::Shutdown()
	{
	}

	Buffer FileSystem::ReadBinary(const std::filesystem::path& filePath)
	{
		std::ifstream stream(GetFilesystemPath(filePath), std::ios::ate | std::ios::binary);
		if (!stream)
			return Buffer{};

		uint64_t streamSize = (uint64_t)stream.tellg();
		if (!streamSize)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to ReadBinary from file: {}\n\t Error: {}", filePath, strerror(errno));
			return Buffer{};
		}

		stream.seekg(0, std::ios::beg);

		Buffer filedata;
		filedata.Allocate(streamSize);
		stream.read(filedata.As<char>(), streamSize);
		return filedata;
	}

	std::string FileSystem::ReadString(const std::filesystem::path& filePath)
	{
		std::ifstream stream(GetFilesystemPath(filePath));
		if (!stream)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to ReadString from file: {}\n\t Error: {}", filePath, strerror(errno));
			return std::string{};
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	bool FileSystem::WriteBinary(const std::filesystem::path& filePath, Buffer fileData, bool createDirectoriesIfNeeded)
	{
		const auto filesystemPath = GetFilesystemPath(filePath);

		const auto directory = filesystemPath.parent_path();
		if (createDirectoriesIfNeeded && !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);

		std::ofstream stream(filesystemPath, std::ios::binary);
		if (!stream)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to WriteBinary to file: {}\n\t Error: {}", filePath, strerror(errno));
			return false;
		}

		stream.write(fileData.As<const char>(), fileData.Size);
		stream.close();
		return true;
	}

	bool FileSystem::WriteString(const std::filesystem::path& filePath, const std::string& fileData, bool createDirectoriesIfNeeded)
	{
		const auto filesystemPath = GetFilesystemPath(filePath);

		const auto directory = filesystemPath.parent_path();
		if (createDirectoriesIfNeeded && !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);

		std::ofstream stream(filesystemPath);
		if (!stream)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to WriteString to file: {}\n\t Error: {}", filePath, strerror(errno));
			return false;
		}

		stream.write(fileData.data(), fileData.size());
		stream.close();
		return true;
	}

	bool FileSystem::IsValidFilename(std::string_view fileName)
	{
		return fileName.find_first_of("\\/:*?\"<>|") == std::string_view::npos;
	}

	bool FileSystem::AssureUniqueness(std::filesystem::path& path)
	{
		if (!Exists(path))
			return true;

		uint32_t count = 1;
		bool validFilepath = false;
		std::string stem = GetStemString(path);
		std::filesystem::path temp = path;

		while (!validFilepath)
		{
			ReplaceStem(temp, fmt::format("{} ({})", stem, count++));
			validFilepath = !Exists(temp);
		}

		path = temp;
		return false;
	}

	bool FileSystem::AssureUniquenessInDirectory(const std::filesystem::path& directory, std::string& name)
	{
		std::filesystem::path temp = directory / name;
		if (!AssureUniqueness(temp))
		{
			name = GetFilenameString(temp);
			return false;
		}
		return true;
	}

	std::filesystem::path FileSystem::UniquePath(const std::filesystem::path& path)
	{
		std::filesystem::path temp = path;
		AssureUniqueness(temp);
		return temp;
	}

	bool FileSystem::Exists(const std::filesystem::path& filepath)
	{
		const auto& filesystemPath = GetFilesystemPath(filepath);

		std::error_code error;
		bool exists = std::filesystem::exists(filesystemPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to check if path Exists! {}\n\t{}", filesystemPath, error.message());
			SK_HANDLE_FS_ERROR("exists", filesystemPath, error);
			return exists;
		}
		
		return exists;
	}

	bool FileSystem::Exists(const std::filesystem::path& filepath, std::string& errorMsg)
	{
		const auto filesystemPath = GetFilesystemPath(filepath);

		std::error_code error;
		bool exists = std::filesystem::exists(filesystemPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to check if path Exists! {}\n\t{}", filesystemPath, error.message());
			errorMsg = error.message();
			return exists;
		}

		errorMsg.clear();
		return exists;
	}

	std::filesystem::path FileSystem::WorkingDirectory()
	{
		return std::filesystem::current_path();
	}

	std::filesystem::path FileSystem::Absolute(const std::filesystem::path& filepath)
	{
		std::error_code error;
		auto result = std::filesystem::absolute(filepath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to get absolute path! {}\n\t{}", filepath, error.message());
			SK_HANDLE_FS_ERROR("absolute", filepath, error);
		}
		return result;
	}

	std::filesystem::path FileSystem::Relative(const std::filesystem::path& filepath, const std::filesystem::path& base)
	{
		std::error_code error;
		auto result = std::filesystem::relative(filepath, base, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to get relative path!\n\tPath: {}\n\tBase:\n\t{}", filepath, base, error.message());
			SK_HANDLE_FS_ERROR("relative", filepath, base, error);
		}
		return result;
	}

	bool FileSystem::CreateFile(const std::filesystem::path& filepath, bool overrideExisiting)
	{
		return Platform::CreateFile(GetFilesystemPath(filepath), overrideExisiting);
	}

	bool FileSystem::CreateFile(const std::filesystem::path& filepath, bool overrideExisiting, std::string& errorMsg)
	{
		return Platform::CreateFile(GetFilesystemPath(filepath), overrideExisiting, errorMsg);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
	{
		return CopyFile(source, destination, std::filesystem::copy_options::none);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options)
	{
		auto fsSource = GetFilesystemPath(source);
		auto fsDestination = GetFilesystemPath(destination);

		std::error_code error;
		bool copied = std::filesystem::copy_file(fsSource, fsDestination, options, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to copy file! {} => {}\n\t{}", fsSource, fsDestination, error.message());
			SK_HANDLE_FS_ERROR("copy_file", fsDestination, fsDestination, error);
		}

		SK_CORE_TRACE_TAG("Filesystem", "CopyFile {} => {}", source, destination);
		return copied;
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::string& errorMsg)
	{
		return CopyFile(source, destination, std::filesystem::copy_options::none, errorMsg);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options, std::string& errorMsg)
	{
		auto fsSource = GetFilesystemPath(source);
		auto fsDestination = GetFilesystemPath(destination);

		SK_CORE_ASSERT(std::filesystem::exists(fsSource));
		SK_CORE_ASSERT(std::filesystem::exists(fsDestination.parent_path()));

		std::error_code error;
		bool copied = std::filesystem::copy_file(fsSource, fsDestination, options, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to copy file! {} => {}\n\t{}", fsSource, fsDestination, error.message());
			errorMsg = error.message();
			return copied;
		}

		SK_CORE_TRACE_TAG("Filesystem", "CopyFile {} => {}", source, destination);
		errorMsg.clear();
		return copied;
	}

	bool FileSystem::CreateDirectory(const std::filesystem::path& path)
	{
		auto fsPath = GetFilesystemPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directory(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to create directory! {}\n\t{}", fsPath, error.message());
			SK_HANDLE_FS_ERROR("create_directory", fsPath, error);
		}
		return created;
	}

	bool FileSystem::CreateDirectory(const std::filesystem::path& path, std::string& errorMsg)
	{
		auto fsPath = GetFilesystemPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directory(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to create directory! {}\n\t{}", fsPath, error.message());
			errorMsg = error.message();
			return created;
		}

		errorMsg.clear();
		return created;
	}

	bool FileSystem::CreateDirectories(const std::filesystem::path& path)
	{
		auto fsPath = GetFilesystemPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directories(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to create directories! {}\n\t{}", fsPath, error.message());
			SK_HANDLE_FS_ERROR("create_directories", fsPath, error);
		}
		return created;
	}

	bool FileSystem::CreateDirectories(const std::filesystem::path& path, std::string& errorMsg)
	{
		auto fsPath = GetFilesystemPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directories(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to create directories! {}\n\t{}", fsPath, error.message());
			errorMsg = error.message();
			return created;
		}

		errorMsg.clear();
		return created;
	}

	bool FileSystem::Rename(const std::filesystem::path& filepath, const std::string& newName)
	{
		if (!FileSystem::IsValidFilename(newName))
		{
			SK_CORE_ERROR_TAG("Filesystem", "Rename Failed! {} => {}\n\tInvalid Filename", filepath, newName);
			return false;
		}

		std::filesystem::path newPath = FileSystem::ChangeFilename(filepath, newName);

		std::error_code error;
		std::filesystem::rename(filepath, newPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to rename! {} => {}\n\t{}", filepath, newName, error.message());
			SK_HANDLE_FS_ERROR("rename", filepath, newName, error);
			return false;
		}

		SK_CORE_TRACE_TAG("Filesystem", "Rename {} => {} ({})", filepath, newName, newPath);
		return true;
	}

	bool FileSystem::Rename(const std::filesystem::path& filepath, const std::string& newName, std::string& errorMsg)
	{
		if (!FileSystem::IsValidFilename(newName))
		{
			SK_CORE_ERROR_TAG("Filesystem", "Rename Failed! {} => {}\n\tInvalid Filename", filepath, newName);
			errorMsg = "Invalid Filename";
			return false;
		}

		std::filesystem::path newPath = FileSystem::ChangeFilename(filepath, newName);

		std::error_code error;
		std::filesystem::rename(filepath, newPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Rename Failed! {} => {}\n\t{}", filepath, newName, error.message());
			errorMsg = error.message();
			return false;
		}

		SK_CORE_TRACE_TAG("Filesystem", "Rename {} => {} ({})", filepath, newName, newPath);
		errorMsg.clear();
		return true;
	}

	bool FileSystem::Move(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
	{
		std::error_code error;
		std::filesystem::rename(oldPath, newPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Move Failed! {} => {}\n\t{}", oldPath, newPath, error.message());
			SK_HANDLE_FS_ERROR("rename", oldName, newName, error);
			return false;
		}

		SK_CORE_TRACE_TAG("Filesystem", "Move {} => {}", oldPath, newPath);
		return true;
	}

	bool FileSystem::Move(const std::filesystem::path& oldPath, const std::filesystem::path& newPath, std::string& errorMsg)
	{
		std::error_code error;
		std::filesystem::rename(oldPath, newPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Move Failed! {} => {}\n\t{}", oldPath, newPath, error.message());
			errorMsg = error.message();
			return false;
		}

		SK_CORE_TRACE_TAG("Filesystem", "Move {} => {}", oldPath, newPath);
		errorMsg.clear();
		return true;
	}

	bool FileSystem::Remove(const std::filesystem::path& path)
	{
		std::error_code error;
		const bool deleted = std::filesystem::remove(path, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Remove failed! {}\n\t{}", path, error.message());
			SK_HANDLE_FS_ERROR("remove", path, error);
		}

		SK_CORE_TRACE_TAG("Filesystem", "Remove {}", path);
		return deleted;
	}

	bool FileSystem::Remove(const std::filesystem::path& path, std::string& errorMsg)
	{
		std::error_code error;
		const bool deleted = std::filesystem::remove(path, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Remove failed! {}\n\t{}", path, error.message());
			errorMsg = error.message();
			return deleted;
		}

		SK_CORE_TRACE_TAG("Filesystem", "Remove {}", path);
		errorMsg.clear();
		return deleted;
	}

	uint64_t FileSystem::RemoveAll(const std::filesystem::path& path)
	{
		std::error_code error;
		const uintmax_t deleted = std::filesystem::remove_all(path, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Remove all failed! {}\n\t{}", path, error.message());
			SK_HANDLE_FS_ERROR("remove_all", path, error);
		}

		SK_CORE_TRACE_TAG("Filesystem", "RemoveAll {}", path);
		return (uint64_t)deleted;
	}

	uint64_t FileSystem::RemoveAll(const std::filesystem::path& path, std::string& errorMsg)
	{
		std::error_code error;
		const uintmax_t deleted = std::filesystem::remove_all(path, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Remove all failed! {}\n\t{}", path, error.message());
			errorMsg = error.message();
			return (uint64_t)deleted;
		}

		SK_CORE_TRACE_TAG("Filesystem", "RemoveAll {}", path);
		errorMsg.clear();
		return (uint64_t)deleted;
	}

	void FileSystem::TruncateFile(const std::filesystem::path& filepath)
	{
		std::ofstream fout{ GetFilesystemPath(filepath), std::ios::trunc };
		fout.flush();
		fout.close();
	}

	bool FileSystem::IsInDirectory(const std::filesystem::path& directory, const std::filesystem::path& path)
	{
		const auto filesystemDirectory = GetFilesystemPath(directory);
		if (!std::filesystem::is_directory(filesystemDirectory))
			return false;

		auto filesystemPath = GetFilesystemPath(path);
		return filesystemDirectory == filesystemPath.parent_path();
	}

	uint64_t FileSystem::GetLastWriteTime(const std::filesystem::path& path)
	{
		std::error_code error;
		uint64_t lastWriteTime = std::filesystem::last_write_time(path, error).time_since_epoch().count();
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "GetLastWriteTime failed! {}\n\t{}", path, error.message());
			SK_HANDLE_FS_ERROR("last_write_time", path, error);
		}
		return lastWriteTime;
	}

	void FileSystem::RemoveExtension(std::filesystem::path& path)
	{
		path.replace_extension();
	}

	void FileSystem::ReplaceExtension(std::filesystem::path& path, const std::string& extension)
	{
		path.replace_extension(extension);
	}

	void FileSystem::ReplaceStem(std::filesystem::path& path, const std::string& stem)
	{
		std::filesystem::path filename = stem;
		filename.replace_extension(path.extension());
		path.replace_filename(filename);
	}

	void FileSystem::ReplaceFilename(std::filesystem::path& path, const std::string& filename)
	{
		path.replace_filename(filename);
	}

	std::filesystem::path FileSystem::ChangeExtension(const std::filesystem::path& path, const std::string& extesnion)
	{
		std::filesystem::path temp = path;
		ReplaceExtension(temp, extesnion);
		return temp;
	}

	std::filesystem::path FileSystem::ChangeFilename(const std::filesystem::path& path, const std::string& filename)
	{
		std::filesystem::path temp = path;
		ReplaceFilename(temp, filename);
		return temp;
	}

	std::filesystem::path FileSystem::GetParent(const std::filesystem::path& path)
	{
		return path.parent_path();
	}

	std::filesystem::path FileSystem::GetExtension(const std::filesystem::path& path)
	{
		return path.extension();
	}

	std::string FileSystem::GetStemString(const std::filesystem::path& path)
	{
		return path.stem().string();
	}

	std::string FileSystem::GetExtensionString(const std::filesystem::path& path)
	{
		return path.extension().string();
	}

	std::string FileSystem::GetFilenameString(const std::filesystem::path& path)
	{
		return path.filename().string();
	}

	std::string FileSystem::GetParentString(const std::filesystem::path& path)
	{
		return path.parent_path().string();
	}

	std::filesystem::path FileSystem::CreatePath(const std::filesystem::path& directory, const std::string& name, const std::string& extension)
	{
		auto path = directory;
		path.replace_filename(name);
		path.replace_extension(extension);
		return path;
	}

	std::string FileSystem::CreatePathString(const std::filesystem::path& directory, const std::string& name, const std::string& extension)
	{
		return CreatePath(directory, name, extension).string();
	}

	std::filesystem::path FileSystem::GetFilesystemPath(const std::filesystem::path& path)
	{
		return std::filesystem::absolute(path);
	}

}
