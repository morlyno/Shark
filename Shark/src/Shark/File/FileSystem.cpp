#include "skpch.h"
#include "FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"

#include <fmt/os.h>

namespace Shark {

	static Ref<FileWatcher> s_FileWatcher;
	static FileWatcherCallbackFunc s_Callback = nullptr;

	#define ASSET_DIRECTORY_KEY "AssetsDirectory"

	void FileSystem::Init()
	{
		s_FileWatcher = FileWatcher::Create();
	}

	void FileSystem::Shutdown()
	{
		s_FileWatcher = nullptr;
		s_Callback = nullptr;
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

	std::filesystem::path FileSystem::MakeFreeFilePath(const std::filesystem::path& directory, const std::filesystem::path& fileName)
	{
		SK_CORE_ASSERT(directory.is_absolute());
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
		SK_CORE_ASSERT(fsPath.is_absolute());
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

	Buffer FileSystem::ReadBinary(const std::filesystem::path& filePath)
	{
		Buffer buffer;
		std::ifstream fin(filePath, std::ios::binary);
		if (fin)
		{
			fin.seekg(0, std::ios::end);
			size_t fileSize = fin.tellg();
			fin.seekg(0, std::ios::beg);
			buffer.Allocate((uint32_t)fileSize);
			fin.read(buffer.As<char>(), fileSize);
		}
		return buffer;
	}

	void FileSystem::TruncateFile(const std::filesystem::path& filePath)
	{
		std::ofstream fout{ filePath, std::ios::trunc };
		fout.flush();
		fout.close();
	}

}
