#include "skpch.h"
#include "Shark/File/FileWatcher.h"
#include "Shark/Core/Assert.h"

#include "Platform/Windows/WindowsUtility.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Instrumentor.h"

#if SK_PLATFORM_WINDOWS

namespace Shark {

	FileWatcher::FileWatcher()
	{
		OnRename = [](const std::filesystem::path&, const std::filesystem::path&) {};
		OnChanged = OnCreated = OnDeleted = [](const std::filesystem::path&) {};
	}

	FileWatcher::FileWatcher(const std::filesystem::path& directoryPath, bool watchSubTrees)
		: m_Directory(directoryPath), m_WatchSubTrees(watchSubTrees)
	{
		OnRename = [](const std::filesystem::path&, const std::filesystem::path&) {};
		OnChanged = OnCreated = OnDeleted = [](const std::filesystem::path&) {};
	}

	FileWatcher::~FileWatcher()
	{
		if (m_Running)
			Stop();
	}

	void FileWatcher::Start()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(!m_Directory.empty());
		SK_CORE_ASSERT(FileSystem::Exists(m_Directory));


		SK_CORE_VERIFY(!m_Running, "FileWatcher::Start was called but FileWatcher is allready running");
		if (!m_Running)
		{
			SK_CORE_INFO("FileWatcher Started watching: {}", m_Directory);
			m_Running = true;
			m_Thread = std::thread(&FileWatcher::StartThread, this);
		}
	}

	void FileWatcher::Stop()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(m_Running, "FileWatcher::Stop was called but FileWatcher isn't running");
		if (m_Running)
		{
			m_Running = false;
			SetEvent(m_Win32_StopEvent);
			m_Thread.join();
			SK_CORE_INFO("FileWatcher Stoped watching: {}", m_Directory);
		}
	}

	void FileWatcher::StartThread()
	{
		HANDLE directoryHandle = CreateFileW(
			m_Directory.c_str(),
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
			SK_CORE_ERROR("Create File Failed! Win32 error msg: {} ", msg);
			SK_CORE_ASSERT(false);
			return;
		}

		DWORD buffer[2048];
		BOOL watchSubTrees = (BOOL)m_WatchSubTrees;
		DWORD bytesReturned = 0;
		FILE_NOTIFY_INFORMATION* notify = nullptr;
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
			SK_CORE_ERROR("Create Event Failed! Win32 error msg: {}", msg);
			SK_CORE_ASSERT(false);

			// Clean up
			CloseHandle(directoryHandle);
			return;
		}


		HANDLE events[2];
		events[0] = overlapped.hEvent;
		events[1] = CreateEventW(NULL, TRUE, FALSE, NULL);
		m_Win32_StopEvent = events[1];
		if (m_Win32_StopEvent == NULL)
		{
			DWORD error = GetLastError();
			std::string msg = GetLastErrorMsg(error);
			SK_CORE_ERROR("Create Event Failed! Win32 error msg: {}", msg);

			// Clean up
			CloseHandle(overlapped.hEvent);
			CloseHandle(directoryHandle);
			return;
		}

		std::filesystem::path filePath;

		FILE_NOTIFY_INFORMATION* prev = NULL;
		bool prevWasOldName = false;

		while (m_Running)
		{
			BOOL result = ReadDirectoryChangesW(
				directoryHandle,
				buffer,
				sizeof(buffer),
				watchSubTrees,
				notifyFlags,
				&bytesReturned,
				&overlapped,
				nullptr
			);
			
			DWORD event = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (event == WAIT_OBJECT_0 + 1)
				continue;

			SK_CORE_VERIFY(result, fmt::format("Failed to Read Changes! Win32 error msg: {}", GetLastErrorMsg(GetLastError())));
			if (!result)
				continue;

			offset = 0;

			do
			{
				notify = (FILE_NOTIFY_INFORMATION*)((byte*)buffer + offset);
				offset += notify->NextEntryOffset;

				const size_t length = notify->FileNameLength / sizeof(WCHAR);

				filePath = m_Directory / std::wstring(notify->FileName, length);

				switch (notify->Action)
				{
					case FILE_ACTION_RENAMED_OLD_NAME:    prevWasOldName = true; prev = notify; break;
					case FILE_ACTION_RENAMED_NEW_NAME:    SK_CORE_ASSERT(prevWasOldName, "I currently assume that FILE_ACTION_RENAME_OLD_NAME always happends befor FILE_ACTION_RENAME_NEW_NAME");
						                                  OnRename(filePath, std::wstring(prev->FileName, prev->FileNameLength / sizeof(WCHAR))); break;

					case FILE_ACTION_MODIFIED:            OnChanged(filePath); break;
					case FILE_ACTION_ADDED:               OnCreated(filePath); break;
					case FILE_ACTION_REMOVED:             OnDeleted(filePath); break;
					default: SK_CORE_ASSERT(false, "Invalid Action"); break;
				}
			} while (notify->NextEntryOffset);
		}

		// Clean up
		CloseHandle(directoryHandle);

		CloseHandle(events[0]);
		CloseHandle(events[1]);
	}

}

#endif
