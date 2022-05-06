#include "skpch.h"
#include "FileSystem.h"
#include "Shark/Utils/PlatformUtils.h"

#include <fmt/os.h>

namespace Shark {

	static Ref<FileWatcher> s_FileWatcher;

	namespace utils {

		static void AssureInsteance()
		{
			if (!s_FileWatcher)
			{
				FileWatcherSpecification specs{};
				specs.WatchSubTrees = true;
				specs.NotifyFlags = NotifyFlag::All;
				s_FileWatcher = FileWatcher::Create(specs);
			}
		}

	}

	void FileSystem::StartWatching()
	{
		utils::AssureInsteance();

		s_FileWatcher->Start();
	}

	void FileSystem::StartWatching(const std::filesystem::path& directory)
	{
		utils::AssureInsteance();

		s_FileWatcher->SetDirectory(directory, false);
		s_FileWatcher->Start();
	}

	void FileSystem::StartWatching(const std::filesystem::path& directory, FileWatcherCallbackFunc callback)
	{
		utils::AssureInsteance();

		s_FileWatcher->SetDirectory(directory, false);
		s_FileWatcher->SetCallback(callback);
		s_FileWatcher->Start();
	}

	void FileSystem::StartWatching(const FileWatcherSpecification& specs)
	{
		utils::AssureInsteance();

		s_FileWatcher->SetSpecification(specs);
		s_FileWatcher->Start();
	}

	void FileSystem::StopWatching()
	{
		utils::AssureInsteance();

		s_FileWatcher->Stop();
	}

	void FileSystem::PauseWatching()
	{
		utils::AssureInsteance();

		s_FileWatcher->Pause();
	}

	void FileSystem::ContinueWatching()
	{
		utils::AssureInsteance();

		s_FileWatcher->Continue();
	}

	void FileSystem::SkipNextFileEvent()
	{
		utils::AssureInsteance();

		s_FileWatcher->SkipNextEvent();
	}

	void FileSystem::SetFileWatcherCallback(FileWatcherCallbackFunc callback)
	{
		utils::AssureInsteance();

		s_FileWatcher->SetCallback(callback);
	}

	Shark::Ref<Shark::FileWatcher> FileSystem::GetFileWatcher()
	{
		utils::AssureInsteance();

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

}
