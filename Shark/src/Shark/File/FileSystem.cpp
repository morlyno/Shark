#include "skpch.h"
#include "FileSystem.h"

#include "Shark/Core/Project.h"
#include "Shark/Utils/String.h"
#include "Shark/Utils/PlatformUtils.h"

#include <fmt/os.h>

namespace Shark {

	struct FileWatchData
	{
		Scope<filewatch::FileWatch<std::filesystem::path>> m_FileWatch;
		FileWatchCallbackFn m_Callback = nullptr;
		std::vector<FileEvent> m_Events;
	};

	static std::unordered_map<std::filesystem::path, FileWatchData> s_FileWatches;
	static std::mutex s_FileWatchMutex;

	namespace utils {

		static void FileWatchCallback(const std::filesystem::path& key, const std::filesystem::path& file, const filewatch::Event event_type)
		{
			std::unique_lock lock(s_FileWatchMutex);
			auto& watchData = s_FileWatches.at(key);
			watchData.m_Events.emplace_back(FileEvent{ event_type, file });
		}

	}

	void FileSystem::Initialize()
	{
	}

	void FileSystem::Shutdown()
	{
	}

	void FileSystem::ProcessEvents()
	{
		std::unique_lock lock(s_FileWatchMutex);

		for (auto& [path, watchData] : s_FileWatches)
		{
			if (watchData.m_Events.empty())
				continue;

			SK_CORE_VERIFY(watchData.m_Callback);
			SK_CORE_TRACE_TAG("Filesystem", "{} File Events detected\n\t{}", watchData.m_Events.size(), fmt::join(watchData.m_Events, "\n\t"));
			watchData.m_Callback(watchData.m_Events);
			watchData.m_Events.clear();
		}
	}

	void FileSystem::StartWatch(const std::filesystem::path& watchPath, FileWatchCallbackFn callback)
	{
		if (s_FileWatches.contains(watchPath))
		{
			SK_CORE_WARN_TAG("Filesystem", "FileWatch for Path/Directory {} already exists", watchPath);
			return;
		}

		auto& watchData = s_FileWatches[watchPath];
		watchData.m_FileWatch = Scope<filewatch::FileWatch<std::filesystem::path>>::Create(watchPath, std::bind(&utils::FileWatchCallback, watchPath, std::placeholders::_1, std::placeholders::_2));
		watchData.m_Callback = callback;

		SK_CORE_INFO_TAG("Filesystem", "Started Watching {}", watchPath);
	}

	void FileSystem::StartWatch(const std::filesystem::path& watchPath, std::wregex regex, FileWatchCallbackFn callback)
	{
		if (s_FileWatches.contains(watchPath))
		{
			SK_CORE_WARN_TAG("Filesystem", "FileWatch for Path/Directory {} already exists", watchPath);
			return;
		}

		auto& watchData = s_FileWatches[watchPath];
		watchData.m_FileWatch = Scope<filewatch::FileWatch<std::filesystem::path>>::Create(watchPath, regex, std::bind(&utils::FileWatchCallback, watchPath, std::placeholders::_1, std::placeholders::_2));
		watchData.m_Callback = callback;

		SK_CORE_INFO_TAG("Filesystem", "Started Watching {}", watchPath);
	}

	void FileSystem::StopWatch(const std::filesystem::path& watchPath)
	{
		if (!s_FileWatches.contains(watchPath))
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to stop FileWatch! No FileWatch for Path/Directory {}", watchPath);
			return;
		}

		s_FileWatches.erase(watchPath);
		SK_CORE_INFO_TAG("Filesystem", "Stoped Watching {}", watchPath);
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

	bool FileSystem::Exists(const std::filesystem::path& filepath)
	{
		const auto& filesystemPath = GetFilesystemPath(filepath);

		std::error_code error;
		bool exists = std::filesystem::exists(filesystemPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to check if path Exists! {}", filesystemPath);
			throw std::filesystem::filesystem_error("exists", filesystemPath, error);
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
			SK_CORE_ERROR_TAG("Filesystem", "Failed to check if path Exists! {}", filesystemPath);
			errorMsg = error.message();
			return exists;
		}

		errorMsg.clear();
		return exists;
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
			throw std::filesystem::filesystem_error("copy_file", fsDestination, fsDestination, error);
		}

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

		errorMsg.clear();
		return copied;
	}

	bool FileSystem::CreateDirectories(const std::filesystem::path& path)
	{
		auto fsPath = GetFilesystemPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directories(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to create directories! {}\n\t{}", fsPath, error.message());
			throw std::filesystem::filesystem_error("create_directories", fsPath, error);
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

	void FileSystem::Rename(const std::filesystem::path& oldName, const std::filesystem::path& newName)
	{
		std::error_code error;
		std::filesystem::rename(oldName, newName, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to rename! {} => {}\n\t{}", oldName, newName, error.message());
			throw std::filesystem::filesystem_error("rename", oldName, newName, error);
		}
	}

	void FileSystem::Rename(const std::filesystem::path& oldName, const std::filesystem::path& newName, std::string& errorMsg)
	{
		std::error_code error;
		std::filesystem::rename(oldName, newName, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("Filesystem", "Failed to rename! {} => {}\n\t{}", oldName, newName, error.message());
			errorMsg = error.message();
			return;
		}

		errorMsg.clear();
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

	std::filesystem::path FileSystem::GetFilesystemPath(const std::filesystem::path& path)
	{
		return std::filesystem::absolute(path);
	}

	void FileSystem::RemoveExtension(std::filesystem::path& path)
	{
		path.replace_extension();
	}

	void FileSystem::ReplaceExtension(std::filesystem::path& path, const std::string& extension)
	{
		path.replace_extension(extension);
	}

	std::filesystem::path FileSystem::ReplaceExtension(const std::filesystem::path& filepath, const std::string& extension)
	{
		std::filesystem::path temp = filepath;
		ReplaceExtension(temp, extension);
		return temp;
	}

	std::string FileSystem::GetStemString(const std::filesystem::path& path)
	{
		return path.stem().string();
	}

	std::string ToString(filewatch::Event event)
	{
		return filewatch::event_to_string(event);
	}

}
