#include "skpch.h"
#include "EditorAssetThread.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"
#include <ranges>

namespace Shark {

	EditorAssetThread::EditorAssetThread()
		: m_Thread("AssetThread")
	{
		m_IdleSignal = CreateEvent(nullptr, true, false, nullptr);
		m_Thread.Dispacht(&EditorAssetThread::AssetThreadFunc, this);
	}

	EditorAssetThread::~EditorAssetThread()
	{
		Stop();
		CloseHandle(m_IdleSignal);
	}

	void EditorAssetThread::Stop()
	{
		m_Running = false;
		m_WorkAvailable.notify_all();
		m_Thread.Join();
	}

	void EditorAssetThread::WaitUntilIdle()
	{
		SK_CORE_WARN_TAG("AssetThread", "Waiting until idle");
		WaitForSingleObject(m_IdleSignal, INFINITE);
	}

	void EditorAssetThread::QueueAssetLoad(const AssetLoadRequest& alr)
	{
		SK_CORE_TRACE_TAG("AssetThread", "QueueAssetLoad - {}", alr.Metadata.FilePath);
		std::lock_guard lock(m_LoadingQueueMutex);
		m_LoadingQueue.push_back(alr);
		
		m_WorkAvailable.notify_one();
	}

	Threading::Future<Ref<Asset>> EditorAssetThread::GetFuture(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_LoadingQueueMutex);
			for (auto& alr : m_LoadingQueue)
			{
				if (alr.Metadata.Handle == handle)
					return alr.Future;
			}
		}

		{
			std::scoped_lock lock(m_LoadedAssetsMutex);
			for (auto& alr : m_LoadedAssets)
			{
				if (alr.Metadata.Handle == handle)
					return alr.Future;
			}
		}

		return {};
	}

	AssetMetaData EditorAssetThread::GetLoadedAssetMetadata(AssetHandle handle)
	{
		std::scoped_lock lock(m_LoadedAssetsMutex);
		for (const auto& alr : std::ranges::reverse_view(m_LoadedAssets))
		{
			if (alr.Metadata.Handle == handle)
				return alr.Metadata;
		}
		
		return {};
	}

	void EditorAssetThread::RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets)
	{
		std::scoped_lock lock(m_LoadedAssetsMutex);
		if (m_LoadedAssets.empty())
			return;

		for (auto& alr : m_LoadedAssets)
			alr.Future.Signal();

		outLoadedAssets = m_LoadedAssets;
		m_LoadedAssets.clear();
	}

	void EditorAssetThread::UpdateLoadedAssetsMetadata(const LoadedAssetsMap& loadedAssets, const AssetRegistry& registry)
	{
		std::scoped_lock lock(m_LoadedAssetMetadataMutex);

		m_LoadedAssetMetadata.clear();
		for (auto& [handle, asset] : loadedAssets)
		{
			const auto& metadata = registry.Get(handle);
			m_LoadedAssetMetadata.push_back(metadata);
		}
	}

	void EditorAssetThread::AssetThreadFunc()
	{
		SK_PROFILE_FRAME("AssetThread");
		while (m_Running)
		{
			if (m_LoadingQueue.empty())
			{
				SetEvent(m_IdleSignal);

				SK_PROFILE_SCOPED("Idle");
				std::unique_lock lock(m_WorkAvailableMutex);
				if (m_MonitorAssets)
					m_WorkAvailable.wait_for(lock, m_MonitorAssetsIntervall);
				else
					m_WorkAvailable.wait(lock);

				ResetEvent(m_IdleSignal);
			}

			SK_PROFILE_SCOPED("Busy");
			std::vector<AssetLoadRequest> loadQueue;

			{
				std::unique_lock lock(m_LoadingQueueMutex);
				std::swap(m_LoadingQueue, loadQueue);
			}

			SK_LOG_IF(loadQueue.size() > 0, LoggerType::Core, LogLevel::Trace, "AssetThread", "Loading {} Assets", loadQueue.size());

			for (AssetLoadRequest& alr : loadQueue)
			{
				SK_PROFILE_SCOPED("Loading Asset");
				SK_CORE_INFO_TAG("AssetThread", "Loading asset {}", alr.Metadata.FilePath);

				bool success = AssetSerializer::TryLoadAsset(alr.Asset, alr.Metadata);
				if (success)
				{
					auto absolutePath = GetFilesystemPath(alr.Metadata);
					alr.Metadata.LastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
					alr.Metadata.Status = AssetStatus::Ready;

					{
						std::scoped_lock lock(m_LoadedAssetsMutex);
						m_LoadedAssets.emplace_back(alr);
					}

					alr.Future.Set(alr.Asset);
					alr.Future.Signal(true, false);
				}

				SK_CORE_INFO_TAG("AssetThread", "Finished loading Asset {}", alr.Metadata.FilePath);
			}

			if (m_MonitorAssets)
			{
				MonitorAssets();
			}
		}
	}

	void EditorAssetThread::MonitorAssets()
	{
		SK_PROFILE_FUNCTION();

		std::unique_lock lock(m_LoadedAssetMetadataMutex);
		for (auto& metadata : m_LoadedAssetMetadata)
		{
			EnsureCurrent(metadata);
		}
	}

	bool EditorAssetThread::EnsureCurrent(AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		// Asset was already reloaded
		if (metadata.Status == AssetStatus::Loading)
			return false;

		auto absolutePath = GetFilesystemPath(metadata);

		if (!FileSystem::Exists(absolutePath))
			return false;

		uint64_t recordedLastWriteTime = metadata.LastWriteTime;
		uint64_t actualLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);

		if (recordedLastWriteTime == 0 || actualLastWriteTime == 0)
			return false;

		if (recordedLastWriteTime != actualLastWriteTime)
		{
			SK_CORE_WARN_TAG("AssetThread", "Out of date asset detected {}", metadata.FilePath);
			metadata.Status = AssetStatus::Loading;
			AssetLoadRequest alr(metadata, true);
			QueueAssetLoad(alr);
			return true;
		}

		return false;
	}

	std::filesystem::path EditorAssetThread::GetFilesystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::GetFilesystemPath(metadata.FilePath);

		return (Project::GetActiveAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

}
