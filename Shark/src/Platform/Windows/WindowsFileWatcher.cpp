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

		static EventFilter::Type ActionToEventFilter(DWORD fileAction)
		{
			switch (fileAction)
			{
				case 0:                            return EventFilter::None;
				case FILE_ACTION_ADDED:            return EventFilter::Created;
				case FILE_ACTION_REMOVED:          return EventFilter::Deleted;
				case FILE_ACTION_MODIFIED:         return EventFilter::Modified;
				case FILE_ACTION_RENAMED_OLD_NAME: return EventFilter::Renamed;
				case FILE_ACTION_RENAMED_NEW_NAME: return EventFilter::Renamed;
			}
			SK_CORE_ASSERT(false, "Unkown FileEvent");
			return EventFilter::None;
		}

	}

	struct WatchData
	{
		OVERLAPPED Overlapped;
		HANDLE DirectoryHandle;
		DWORD Buffer[1024];
		bool IsRecursive;
		DWORD NotifyFilter;
		EventFilter::Flags EnabledEvents;
		wchar_t DirectoryPath[260];
		FileWatcherCallbackFunc Callback;
		bool Stop;
	};

	static void WatchResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	static bool ReadChanges(WatchData* watchData, bool stopWatch = false)
	{
		return ReadDirectoryChangesExW(
			watchData->DirectoryHandle,
			watchData->Buffer,
			sizeof(watchData->Buffer),
			watchData->IsRecursive,
			watchData->NotifyFilter,
			NULL,
			&watchData->Overlapped,
			stopWatch ? NULL : &WatchResult,
			ReadDirectoryNotifyExtendedInformation
		);
	}

	static void WatchResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		WatchData* watchData = (WatchData*)lpOverlapped;

		if (dwErrorCode != ERROR_SUCCESS)
		{
			std::string msg = std::system_category().message(dwErrorCode);
			SK_CORE_ERROR_TAG("FileWatcher", "Reading Directory Changes failed! {0}", msg);
			SK_CORE_ASSERT(dwNumberOfBytesTransfered == 0);
			return;
		}

		if (dwErrorCode == ERROR_SUCCESS && watchData->Callback)
		{
			sizeof(FILE_NOTIFY_INFORMATION);
			FILE_NOTIFY_EXTENDED_INFORMATION* notify = nullptr;
			DWORD offset = 0;

			std::vector<FileChangedData> fileEvents;

			while (true)
			{
				notify = (FILE_NOTIFY_EXTENDED_INFORMATION*)((byte*)watchData->Buffer + offset);
				offset += notify->NextEntryOffset;

				EventFilter::Type currentEvent = utils::ActionToEventFilter(notify->Action);
				if (watchData->EnabledEvents & currentEvent)
				{
					auto& event = fileEvents.emplace_back();
					event.Type = utils::Win32FileActionToFileEvent(notify->Action);
					event.IsDirectory = notify->FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
					event.FilePath = fmt::format(L"{}/{}", watchData->DirectoryPath, std::wstring(notify->FileName, notify->FileNameLength / sizeof(WCHAR)));
				}

				if (!notify->NextEntryOffset)
					break;
			}

			if (!fileEvents.empty())
			{
				SK_CORE_TRACE_TAG("FileWatcher", "File Events detected\n{0}", fmt::join(fileEvents, "\n"));
				watchData->Callback(fileEvents);
			}
		}

		if (!watchData->Stop)
			ReadChanges(watchData);
	}

	static WatchData* CreateWatchData(LPCWSTR dirPath, DWORD notifyFilter, BOOL isRecursive)
	{
		WatchData* watchData = sknew WatchData;
		memset(watchData, 0, sizeof(WatchData));
		watchData->DirectoryHandle = CreateFileW(
			dirPath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL
		);

		if (watchData->DirectoryHandle == INVALID_HANDLE_VALUE)
		{
			skdelete watchData;
			DWORD lastError = GetLastError();
			std::string msg = std::system_category().message(lastError);
			SK_CORE_ERROR_TAG("FileWatcher", "CreateFileW failed! {}", msg);
			return nullptr;
		}

		watchData->Overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
		watchData->IsRecursive = isRecursive;
		watchData->NotifyFilter = notifyFilter;

		if (ReadChanges(watchData))
			return watchData;

		SK_CORE_ERROR_TAG("FileWatcher", "Failed to Create Watch for '{0}'", String::ToNarrowCopy(dirPath));
		CloseHandle(watchData->DirectoryHandle);
		CloseHandle(watchData->Overlapped.hEvent);
		skdelete watchData;
		return nullptr;
	}

	static void DestroyWatchData(WatchData* watchData)
	{
		if (!watchData)
			return;

		watchData->Stop = true;
		CancelIo(watchData->DirectoryHandle);
		ReadChanges(watchData, true);

		if (!HasOverlappedIoCompleted(&watchData->Overlapped))
			SleepEx(6, TRUE);

		CloseHandle(watchData->DirectoryHandle);
		CloseHandle(watchData->Overlapped.hEvent);
		skdelete watchData;
	}

	WindowsFileWatcher::~WindowsFileWatcher()
	{
		for (auto& [key, watchData] : m_Watches)
			DestroyWatchData(watchData);
	}

	void WindowsFileWatcher::StartWatching(const std::string& key, const std::filesystem::path& dirPath, FileWatcherCallbackFunc callback)
	{
		SK_CORE_ASSERT(m_Watches.find(key) == m_Watches.end());
		WatchData* watchData = CreateWatchData(dirPath.c_str(), (DWORD)NotifyFilter::Default, TRUE);
		if (!watchData)
			return;

		watchData->Callback = callback;
		watchData->EnabledEvents = EventFilter::All;
		wcscpy_s(watchData->DirectoryPath, dirPath.c_str());
		m_Watches.emplace(key, watchData);
		SK_CORE_INFO_TAG("FileWatcher", "Started watching {0} ({1})", key, String::ToNarrowCopy(watchData->DirectoryPath));
	}

	void WindowsFileWatcher::StartWatching(const std::string& key, const std::filesystem::path& dirPath, const WatchingSettings& settings)
	{
		SK_CORE_ASSERT(m_Watches.find(key) == m_Watches.end());
		WatchData* watchData = CreateWatchData(dirPath.c_str(), (DWORD)settings.NofityFilter, settings.IsRecursive);
		if (!watchData)
			return;
		
		watchData->Callback = settings.Callback;
		watchData->EnabledEvents = settings.EnabledEvents;
		wcscpy_s(watchData->DirectoryPath, dirPath.c_str());
		m_Watches.emplace(key, watchData);
		SK_CORE_INFO_TAG("FileWatcher", "Started watching {0} ({1})", key, String::ToNarrowCopy(watchData->DirectoryPath));
	}

	void WindowsFileWatcher::StopWatching(const std::string& key)
	{
		if (m_Watches.find(key) == m_Watches.end())
			return;

		WatchData* watchData = m_Watches.at(key);
		SK_CORE_INFO_TAG("FileWatcher", "Stoped watching {0} ({1})", key, String::ToNarrowCopy(watchData->DirectoryPath));
		m_Watches.erase(key);
		DestroyWatchData(watchData);
	}

	void WindowsFileWatcher::SetCallback(const std::string& key, FileWatcherCallbackFunc callback)
	{
		SK_CORE_ASSERT(m_Watches.find(key) != m_Watches.end());
		m_Watches.at(key)->Callback = callback;
	}

	bool WindowsFileWatcher::IsWatching(const std::string& key)
	{
		auto iter = m_Watches.find(key);
		return iter != m_Watches.end() && !iter->second->Stop;
	}

	void WindowsFileWatcher::Update()
	{
		MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);
	}

	Ref<FileWatcher> FileWatcher::Create()
	{
		return Ref<WindowsFileWatcher>::Create();
	}

}

