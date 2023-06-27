#include "skpch.h"
#include "FileSystem.h"

#include "Shark/Core/Project.h"
#include "Shark/Utils/PlatformUtils.h"

#include <fmt/os.h>
#include "Shark/Utils/String.h"

namespace Shark {

	static Ref<FileWatcher> s_FileWatcher;
	static FileWatcherCallbackFunc s_Callback = nullptr;

	#define ASSET_DIRECTORY_KEY "AssetsDirectory"

	void FileSystem::Initialize()
	{
		s_FileWatcher = FileWatcher::Create();
	}

	void FileSystem::Shutdown()
	{
		SK_CORE_ASSERT(s_FileWatcher->GetActiveCount() == 0);
		s_FileWatcher = nullptr;
		s_Callback = nullptr;
	}

	void FileSystem::ProcessEvents()
	{
		s_FileWatcher->Update();
	}

	void FileSystem::StartWatching(const std::filesystem::path& dirPath)
	{
		s_FileWatcher->StartWatching(ASSET_DIRECTORY_KEY, dirPath, s_Callback);
	}

	void FileSystem::StopWatching()
	{
		s_FileWatcher->StopWatching(ASSET_DIRECTORY_KEY);
	}

	void FileSystem::SetCallback(FileWatcherCallbackFunc callback)
	{
		s_Callback = callback;
		if (s_FileWatcher->IsWatching(ASSET_DIRECTORY_KEY))
			s_FileWatcher->SetCallback(ASSET_DIRECTORY_KEY, callback);

	}

	Ref<FileWatcher> FileSystem::GetFileWatcher()
	{
		return s_FileWatcher;
	}

	std::filesystem::path FileSystem::GetUnsusedPath(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
			return filePath;

		std::filesystem::path path = filePath;
		const std::filesystem::path directory = filePath.parent_path();
		const std::wstring name = filePath.stem();
		const std::wstring extension = filePath.extension();

		uint32_t count = 1;
		bool foundValidFileName = false;
		while (!foundValidFileName)
		{
			path = fmt::format(L"{}/{} ({}).{}", directory, name, count, extension);
			foundValidFileName = std::filesystem::exists(path);
		}

		return path;
	}

	std::filesystem::path FileSystem::MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName)
	{
		SK_CORE_VERIFY(directory.is_absolute());
		std::filesystem::path fsPath = directory / fileName;

		if (std::filesystem::exists(fsPath))
		{
			const std::wstring name = fileName.stem();
			const std::wstring extension = fileName.extension();

			uint32_t count = 1;
			bool foundValidFileName = false;
			while (!foundValidFileName)
			{
				fsPath = fmt::format(L"{}/{} ({}).{}", directory, name, count, extension);
				foundValidFileName = std::filesystem::exists(fsPath);
			}
		}

		return fsPath;
	}

	void FileSystem::MakeFreeFilePath(std::filesystem::path& fsPath)
	{
		SK_CORE_VERIFY(fsPath.is_absolute());
		if (std::filesystem::exists(fsPath))
		{
			const std::wstring directory = fsPath.parent_path();
			const std::wstring name = fsPath.stem();
			const std::wstring extension = fsPath.extension();

			uint32_t count = 1;
			bool foundValidFileName = false;
			while (!foundValidFileName)
			{
				fsPath = fmt::format(L"{}/{} ({}).{}", directory, name, count++, extension);
				foundValidFileName = std::filesystem::exists(fsPath);
			}
		}
	}

	bool FileSystem::IsValidFileName(std::string_view fileName)
	{
		return fileName.find_first_of("\\/:*?\"<>|") == std::string_view::npos;
	}

	bool FileSystem::CreateScriptFile(const std::filesystem::path& directory, const std::string& projectName, const std::string& scriptName)
	{
		const std::string filePath = fmt::format("{}/{}.cs", directory, scriptName);
		std::ofstream fout(filePath);
		if (!fout)
			return false;

		fout << "using Shark;\n";
		fout << "\n";
		fout << "namespace " << projectName << "\n";
		fout << "{\n";
		fout << "\t\n";
		fout << "\tpublic class " << scriptName << " : Entity\n";
		fout << "\t{\n";

		fout << "\t\tvoid OnCreate()\n";
		fout << "\t\t{\n";
		fout << "\t\t\t\n";
		fout << "\t\t}\n";

		fout << "\t\t\n";

		fout << "\t\tvoid OnDestroy()\n";
		fout << "\t\t{\n";
		fout << "\t\t\t\n";
		fout << "\t\t}\n";

		fout << "\t\t\n";

		fout << "\t\tvoid OnUpdate(TimeStep ts)\n";
		fout << "\t\t{\n";
		fout << "\t\t\t\n";
		fout << "\t\t}\n";

		fout << "\t}\n";
		fout << "\t\n";
		fout << "}\n";

		fout.close();
		return true;
	};

	bool FileSystem::CreateFile(const std::filesystem::path& filePath, bool overrideExisiting)
	{
		return PlatformUtils::CreateFile(filePath, overrideExisiting);
	}

	Buffer FileSystem::ReadBinary(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
		if (!stream)
			return Buffer{};

		uint64_t streamSize = (uint64_t)stream.tellg();
		if (!streamSize)
			return Buffer{};

		stream.seekg(0, std::ios::beg);

		Buffer filedata;
		filedata.Allocate(streamSize);
		stream.read(filedata.As<char>(), streamSize);
		return filedata;
	}

	std::string FileSystem::ReadString(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath);
		if (!stream)
			return std::string{};

		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	bool FileSystem::WriteBinary(const std::filesystem::path& filePath, Buffer fileData, bool createDirectoriesIfNeeded)
	{
		const auto directory = filePath.parent_path();
		if (createDirectoriesIfNeeded && !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);

		std::ofstream stream(filePath);
		if (!stream)
			return false;

		stream.write(fileData.As<const char>(), fileData.Size);
		stream.close();
		return true;
	}

	bool FileSystem::WriteString(const std::filesystem::path& filePath, const std::string& fileData, bool createDirectoriesIfNeeded)
	{
		const auto directory = filePath.parent_path();
		if (createDirectoriesIfNeeded && !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);

		std::ofstream stream(filePath);
		if (!stream)
			return false;

		stream.write(fileData.data(), fileData.size());
		stream.close();
		return true;
	}

	void FileSystem::TruncateFile(const std::filesystem::path& filePath)
	{
		std::ofstream fout{ GetFSPath(filePath), std::ios::trunc };
		fout.flush();
		fout.close();
	}

	bool FileSystem::Exists(const std::filesystem::path& filepath)
	{
		if (std::filesystem::exists(filepath))
			return true;

		if (filepath.is_relative() && std::filesystem::exists(Project::GetDirectory() / filepath))
			return true;

		return false;
	}

	std::string FileSystem::ParseFileName(const std::filesystem::path& filePath)
	{
		return filePath.stem().string();
	}

	bool FileSystem::IsInDirectory(const std::filesystem::path& directory, const std::filesystem::path& path)
	{
		auto absolutDir = GetAbsolute(directory);
		if (!std::filesystem::is_directory(directory))
			return false;

		auto pathAbsolut = GetAbsolute(path);
		return absolutDir == pathAbsolut.parent_path();
	}

	std::filesystem::path FileSystem::GetRelative(const std::filesystem::path& path)
	{
		return Project::RelativeCopy(path);
	}

	std::filesystem::path FileSystem::GetAbsolute(const std::filesystem::path& path)
	{
		return Project::AbsolueCopy(path);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
	{
		return CopyFile(source, destination, std::filesystem::copy_options::none);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options)
	{
		auto fsSource = GetFSPath(source);
		auto fsDestination = GetFSPath(destination);

		std::error_code error;
		bool copied = std::filesystem::copy_file(fsSource, fsDestination, options, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("FileSystem", "Failed to copy file! {} => {}\n\t{}", fsSource, fsDestination, error.message());
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
		auto fsSource = GetFSPath(source);
		auto fsDestination = GetFSPath(destination);

		SK_CORE_ASSERT(std::filesystem::exists(fsSource));
		SK_CORE_ASSERT(std::filesystem::exists(fsDestination.parent_path()));

		std::error_code error;
		bool copied = std::filesystem::copy_file(fsSource, fsDestination, options, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("FileSystem", "Failed to copy file! {} => {}\n\t{}", fsSource, fsDestination, error.message());
			errorMsg = error.message();
			return copied;
		}

		errorMsg.clear();
		return copied;
	}

	bool FileSystem::CreateDirectories(const std::filesystem::path& path)
	{
		auto fsPath = GetFSPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directories(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("FileSystem", "Failed to create directories! {}\n\t{}", fsPath);
			throw std::filesystem::filesystem_error("create_directories", fsPath, error);
		}

		return created;
	}

	bool FileSystem::CreateDirectories(const std::filesystem::path& path, std::string& errorMsg)
	{
		auto fsPath = GetFSPath(path);

		std::error_code error;
		bool created = std::filesystem::create_directories(fsPath, error);
		if (error)
		{
			SK_CORE_ERROR_TAG("FileSystem", "Failed to create directories! {}\n\t{}", fsPath);
			errorMsg = error.message();
			return created;
		}

		errorMsg.clear();
		return created;
	}

#if 0
	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
	{
		return CopyFile(source, destination, std::filesystem::copy_options::none);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options)
	{
		auto fsSource = GetAbsolute(source);
		auto fsDestination = GetAbsolute(destination);
		return std::filesystem::copy_file(fsSource, fsDestination, options);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::filesystem::copy_options options, std::error_code& error)
	{
		auto fsSource = GetAbsolute(source);
		auto fsDestination = GetAbsolute(destination);
		return std::filesystem::copy_file(fsSource, fsDestination, options, error);
	}

	bool FileSystem::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& error)
	{
		return CopyFile(source, destination, std::filesystem::copy_options::none, error);
	}
#endif

	std::filesystem::path FileSystem::GetFSPath(const std::filesystem::path& path)
	{
		return path.is_absolute() ? path : std::filesystem::exists(path) ? path : GetAbsolute(path);
	}

}
