#include "skpch.h"
#include "WindowsFileWatcher.h"

#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Utils/String.h"

namespace Shark {

	namespace utils {

		static FileEvent Win32FileActionToFileEvent(DWORD fileAction)
		{
			switch (fileAction)
			{
				case FILE_ACTION_ADDED: return FileEvent::Created;
				case FILE_ACTION_REMOVED: return FileEvent::Deleted;
				case FILE_ACTION_MODIFIED: return FileEvent::Modified;
				case FILE_ACTION_RENAMED_OLD_NAME: return FileEvent::OldName;
				case FILE_ACTION_RENAMED_NEW_NAME: return FileEvent::NewName;
			}
			SK_CORE_ASSERT(false, "Unkown File Action");
			return FileEvent::None;
		}

		static DWORD SharkNotifyFalgsToWin32(NotifyFlag::Type flags)
		{
			DWORD notifyFlags = 0;

			if (flags & NotifyFlag::FileName) notifyFlags |= FILE_NOTIFY_CHANGE_FILE_NAME;
			if (flags & NotifyFlag::DirectoryName) notifyFlags |= FILE_NOTIFY_CHANGE_DIR_NAME;
			if (flags & NotifyFlag::Attributes) notifyFlags |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
			if (flags & NotifyFlag::Size) notifyFlags |= FILE_NOTIFY_CHANGE_SIZE;
			if (flags & NotifyFlag::LastWrite) notifyFlags |= FILE_NOTIFY_CHANGE_LAST_WRITE;
			if (flags & NotifyFlag::LastAccess) notifyFlags |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
			if (flags & NotifyFlag::Creation) notifyFlags |= FILE_NOTIFY_CHANGE_CREATION;
			if (flags & NotifyFlag::Security) notifyFlags |= FILE_NOTIFY_CHANGE_SECURITY;

			return notifyFlags;
		}

	}

	WindowsFileWatcher::WindowsFileWatcher(const FileWatcherSpecification& specification)
		: m_Specs(specification)
	{
	}

	WindowsFileWatcher::~WindowsFileWatcher()
	{
	}

	void WindowsFileWatcher::Start()
	{
		if (m_Running || !std::filesystem::is_directory(m_Specs.Directory))
			return;

		SK_CORE_ASSERT(std::filesystem::is_directory(m_Specs.Directory), "Path for FileWatcher is not a Directory");
		m_Thread = std::thread(std::bind(&WindowsFileWatcher::WatcherFunction, this));
	}

	void WindowsFileWatcher::Stop()
	{
		if (!m_Running || !m_Thread.joinable())
			return;

		m_Running = false;
		SetEvent(m_StopThreadEvent);
		m_Thread.join();
		SK_CORE_INFO(L"File Watcher Stoped Watching {}", m_Specs.Directory);
	}

	void WindowsFileWatcher::SetDirectory(const std::filesystem::path& directory, bool restartIfRunning)
	{
		if (!m_Running)
		{
			m_Specs.Directory = directory;
			return;
		}

		if (restartIfRunning)
		{
			Stop();
			m_Specs.Directory = directory;
			Start();
		}
	}

	void WindowsFileWatcher::WatcherFunction()
	{
		if (m_Running)
			return;

		m_Running = true;

		WindowsUtils::SetThreadName(L"FileWatcher");

		// get directory handle
		HANDLE directoryHandle = CreateFileW(
			m_Specs.Directory.c_str(),
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
			std::string msg = WindowsUtils::TranslateErrorCode(error);
			SK_CORE_ASSERT(false, fmt::format("CreateFileW Failed! Error Msg: {} ", msg));
			return;
		}

		// Notify Flags
		const DWORD notifyFlags = utils::SharkNotifyFalgsToWin32(m_Specs.NotifyFlags);


		// Overlapped and Events
		OVERLAPPED overlapped;
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		overlapped.Offset = 0;
		overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
		if (overlapped.hEvent == NULL)
		{
			DWORD error = GetLastError();
			std::string msg = WindowsUtils::TranslateErrorCode(error);
			SK_CORE_ERROR("Create Event Failed! Error Msg: {}", msg);
			SK_CORE_ASSERT(false);

			// Clean up
			CloseHandle(directoryHandle);
			return;
		}

		HANDLE events[2];
		events[0] = overlapped.hEvent;
		events[1] = CreateEventW(NULL, TRUE, FALSE, NULL);
		m_StopThreadEvent = events[1];
		if (m_StopThreadEvent == NULL)
		{
			DWORD error = GetLastError();
			std::string msg = WindowsUtils::TranslateErrorCode(error);
			SK_CORE_ERROR("Create Event Failed! Error Msg: {}", msg);

			// Clean up
			CloseHandle(overlapped.hEvent);
			CloseHandle(directoryHandle);
			return;
		}


		// Temp Data
		const BOOL watchSubTrees = m_Specs.WatchSubTrees;
		std::vector<FileChangedData> fileChanges;
		DWORD buffer[4096];
		DWORD bytesReturned = 0;
		DWORD offset = 0;
		ZeroMemory(buffer, sizeof(buffer));

		while (m_Running)
		{
			BOOL result = ReadDirectoryChangesExW(
				directoryHandle,
				buffer,
				sizeof(buffer),
				watchSubTrees,
				notifyFlags,
				&bytesReturned,
				&overlapped,
				nullptr,
				ReadDirectoryNotifyExtendedInformation
			);

			DWORD event = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (event == WAIT_OBJECT_0 + 1)
				continue;

			SK_CORE_VERIFY(result, fmt::format("Failed to Read Changes! Error Msg: {}", WindowsUtils::TranslateErrorCode(GetLastError())));
			if (!result)
				continue;

			offset = 0;
			fileChanges.clear();

			if (m_SkipNextEvent || m_Paused)
			{
				m_SkipNextEvent = false;
				continue;
			}

			while (true)
			{
				//FILE_NOTIFY_INFORMATION* fileInfo = (FILE_NOTIFY_INFORMATION*)((byte*)buffer + offset);
				FILE_NOTIFY_EXTENDED_INFORMATION* fileInfo = (FILE_NOTIFY_EXTENDED_INFORMATION*)((byte*)buffer + offset);
				offset += fileInfo->NextEntryOffset;

				const size_t length = fileInfo->FileNameLength / sizeof(WCHAR);

				FileChangedData fileData;
				fileData.FilePath = String::FormatDefaultCopy(m_Specs.Directory / std::wstring(fileInfo->FileName, length));
				fileData.Type = utils::Win32FileActionToFileEvent(fileInfo->Action);
				fileData.IsDirectory = fileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
				fileChanges.push_back(fileData);

				if (!fileInfo->NextEntryOffset)
					break;
			}

			if (m_Specs.CallbackFunc)
				m_Specs.CallbackFunc(fileChanges);
		}

		// Clean up
		CloseHandle(directoryHandle);

		CloseHandle(events[0]);
		CloseHandle(events[1]);

		m_StopThreadEvent = NULL;

		m_Running = false;
	}

}

