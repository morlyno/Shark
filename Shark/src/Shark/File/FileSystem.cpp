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

	void FileSystem::TruncateFile(const std::filesystem::path& filePath)
	{
		std::ofstream fout{ filePath, std::ios::trunc };
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

	std::filesystem::path FileSystem::GetRelative(const std::filesystem::path& path)
	{
		return Project::RelativeCopy(path);
	}

	std::filesystem::path FileSystem::GetAbsolute(const std::filesystem::path& path)
	{
		return Project::AbsolueCopy(path);
	}

}
