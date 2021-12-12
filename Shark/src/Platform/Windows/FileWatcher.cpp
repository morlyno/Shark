#include "skpch.h"
#include "Shark/File/FileWatcher.h"
#include "Shark/Core/Assert.h"

#include "Platform/Windows/WindowsUtility.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Core/Application.h"

#include "Shark/Debug/Instrumentor.h"

#include <algorithm>

#if SK_PLATFORM_WINDOWS

namespace Shark {

	namespace Utils {

		static FileEvent Win32FileActionToFileEvent(DWORD fileAction)
		{
			switch (fileAction)
			{
				case FILE_ACTION_ADDED: return FileEvent::Created;
				case FILE_ACTION_REMOVED: return FileEvent::Deleted;
				case FILE_ACTION_MODIFIED: return FileEvent::Modified;
				case FILE_ACTION_RENAMED_NEW_NAME: return FileEvent::Renamed;
			}
			SK_CORE_ASSERT(false, "Unkown File Action");
			return FileEvent::None;
		}

	}

	struct FileWatcherData
	{
		std::thread Thread;
		std::filesystem::path Directory;
		std::wstring DirectoryName;
		HANDLE StopThreadEvent = NULL;
		bool Running = false;
	};

	static Scope<FileWatcherData> s_Data = nullptr;

	void FileWatcher::StartWatching(const std::filesystem::path& directory)
	{
		if (!s_Data)
			s_Data = Scope<FileWatcherData>::Create();

		SK_CORE_ASSERT(std::filesystem::is_directory(directory), "Path for FileWatcher is not a Directory");
		if (!std::filesystem::is_directory(directory))
			return;

		s_Data->Directory = directory;
		s_Data->DirectoryName = directory.filename();

		s_Data->Thread = std::thread(StartThread);
		SK_CORE_INFO("File Watcher Stated Watching {}", s_Data->Directory);
	}

	void FileWatcher::StopWatching()
	{
		SK_CORE_VERIFY(s_Data);
		if (!s_Data)
			return;

		if (s_Data->Running)
		{
			s_Data->Running = false;
			SetEvent(s_Data->StopThreadEvent);
			s_Data->Thread.join();
			SK_CORE_INFO("File Watcher Stoped Watching {}", s_Data->Directory);
		}
	}

	bool FileWatcher::IsRunning()
	{
		return s_Data ? s_Data->Running : false;
	}

	void FileWatcher::StartThread()
	{
		SK_CORE_ASSERT(!s_Data->Running, "StartFileWatcher was called but FileWachter is allready Running!");
		if (s_Data->Running)
			return;

		s_Data->Running = true;

		HANDLE directoryHandle = CreateFileW(
			s_Data->Directory.c_str(),
			GENERIC_READ,
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);
		if (directoryHandle == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			std::string msg = GetLastErrorMsg(error);
			SK_CORE_ASSERT(false, fmt::format("CreateFileW Failed! Error Msg: {} ", msg));
			return;
		}
		
		DWORD buffer[2048];
		DWORD bytesReturned = 0;
		FILE_NOTIFY_INFORMATION* fileInfo = nullptr;
		DWORD offset = 0;
		
		ZeroMemory(buffer, sizeof(buffer));
		
		constexpr DWORD notifyFlags =
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_SECURITY;
		
		
		OVERLAPPED overlapped;
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		overlapped.Offset = 0;
		overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
		if (overlapped.hEvent == NULL)
		{
			DWORD error = GetLastError();
			std::string msg = GetLastErrorMsg(error);
			SK_CORE_ERROR("Create Event Failed! Error Msg: {}", msg);
			SK_CORE_ASSERT(false);
			
			// Clean up
			CloseHandle(directoryHandle);
			return;
		}
		
		
		HANDLE events[2];
		events[0] = overlapped.hEvent;
		events[1] = CreateEventW(NULL, TRUE, FALSE, NULL);
		s_Data->StopThreadEvent = events[1];
		if (s_Data->StopThreadEvent == NULL)
		{
			DWORD error = GetLastError();
			std::string msg = GetLastErrorMsg(error);
			SK_CORE_ERROR("Create Event Failed! Error Msg: {}", msg);
			
			// Clean up
			CloseHandle(overlapped.hEvent);
			CloseHandle(directoryHandle);
			return;
		}
		
		std::filesystem::path prevFilePath;
		bool prevWasOldName = false;

		while (s_Data->Running)
		{
			BOOL result = ReadDirectoryChangesW(
				directoryHandle,
				buffer,
				sizeof(buffer),
				TRUE,
				notifyFlags,
				&bytesReturned,
				&overlapped,
				nullptr
			);
			
			DWORD event = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (event == WAIT_OBJECT_0 + 1)
				continue;
			
			SK_CORE_VERIFY(result, fmt::format("Failed to Read Changes! Error Msg: {}", GetLastErrorMsg(GetLastError())));
			if (!result)
				continue;
			
			offset = 0;

			do
			{
				fileInfo = (FILE_NOTIFY_INFORMATION*)((byte*)buffer + offset);
				offset += fileInfo->NextEntryOffset;

				const size_t length = fileInfo->FileNameLength / sizeof(WCHAR);
				std::wstring filePath = fmt::format(L"{}/{}", s_Data->DirectoryName, std::wstring(fileInfo->FileName, length));
				std::replace(filePath.begin(), filePath.end(), L'\\', L'/');

				if (fileInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
				{
					prevFilePath = filePath;
					prevWasOldName = true;

					continue;
				}

				FileChangedEvent fileEvent;
				fileEvent.m_FileEvent = Utils::Win32FileActionToFileEvent(fileInfo->Action);
				fileEvent.m_FilePath = filePath;
				
				SK_CORE_ASSERT(SK_ASSERT_CONDITIONAL(fileEvent.m_FileEvent == FileEvent::Renamed, prevWasOldName), "Previos File Action wasn't FILE_ACTION_RENAMED_OLD_NAME!");
				if (fileEvent.m_FileEvent == FileEvent::Renamed && prevWasOldName)
				{
					fileEvent.m_OldFilePath = prevFilePath;
					prevWasOldName = false;
					prevFilePath.clear();
				}

				Application::Get().OnEvent(fileEvent);


			} while (fileInfo->NextEntryOffset);
		}
		
		// Clean up
		CloseHandle(directoryHandle);
		
		CloseHandle(events[0]);
		CloseHandle(events[1]);

		s_Data->StopThreadEvent = NULL;

		s_Data->Running = false;
	}

}

#endif
