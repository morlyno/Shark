#include "skpch.h"
#include "EditorAssetThread.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"
#include <ranges>

namespace Shark {

	EditorAssetThread::EditorAssetThread(const AssetThreadSettings& settings)
		: m_Thread("AssetThread"), m_IdleSignal(true, false)
	{
		m_MonitorAssets = settings.MonitorAssets;

		m_Thread.Dispacht(&EditorAssetThread::AssetThreadFunc, this);
	}

	EditorAssetThread::~EditorAssetThread()
	{
		Stop();
	}

	void EditorAssetThread::Stop()
	{
		m_Running = false;
		m_WorkAvailable.notify_all();
		m_Thread.Join();
	}

	void EditorAssetThread::WaitUntilIdle()
	{
		SK_PROFILE_FUNCTION();
		if (!m_Running)
			return;

		SK_CORE_WARN_TAG("AssetThread", "Waiting until idle");
		m_IdleSignal.Wait();
	}

	void EditorAssetThread::QueueAssetLoad(AssetLoadRequest& alr)
	{
		SK_CORE_TRACE_TAG("AssetThread", "QueueAssetLoad - {} {}", alr.Metadata.Handle, alr.Metadata.FilePath);

		{
			std::scoped_lock lock(m_ALRStorageMutex);
			if (m_ALRStorage.contains(alr.Metadata.Handle))
			{
				SK_CORE_WARN("ALR Merge {} {}", alr.Metadata.Handle, alr.Metadata.FilePath);
				AssetLoadRequest& activeRequest = m_ALRStorage.at(alr.Metadata.Handle);
				activeRequest.Future.MergeCallbacks(alr.Future);
				alr.Future = activeRequest.Future;
			}
			else
			{
				m_ALRStorage[alr.Metadata.Handle] = alr;
				m_LoadingQueue.push(alr.Metadata.Handle);
			}
		}

		m_WorkAvailable.notify_one();
	}

	Threading::Future<Ref<Asset>> EditorAssetThread::GetFuture(AssetHandle handle)
	{
		std::scoped_lock lock(m_ALRStorageMutex);
		if (!m_ALRStorage.contains(handle))
			return {};

		AssetLoadRequest& request = m_ALRStorage.at(handle);
		return request.Future;
	}

	void EditorAssetThread::RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets)
	{
		SK_PROFILE_FUNCTION();

		std::scoped_lock lock(m_ALRStorageMutex);
		if (m_HandledRequests.empty())
			return;

		for (AssetHandle handle : m_HandledRequests)
		{
			AssetLoadRequest& request = m_ALRStorage.at(handle);
			request.Future.Signal();

			outLoadedAssets.push_back(request);
			m_ALRStorage.erase(handle);
		}

		m_HandledRequests.clear();
	}

	void EditorAssetThread::UpdateLastWriteTime(AssetHandle handle, uint64_t lastWriteTime)
	{
		std::scoped_lock lock(m_LoadedAssetMetadataMutex);
		if (m_LoadedAssetMetadata.contains(handle))
		{
			AssetMetaData& metadata = m_LoadedAssetMetadata.at(handle);
			metadata.LastWriteTime = lastWriteTime;
		}
	}

	void EditorAssetThread::OnMetadataChanged(const AssetMetaData& metadata)
	{
		if (metadata.IsMemoryAsset)
			return;

		std::scoped_lock lock(m_LoadedAssetMetadataMutex);
		if (m_LoadedAssetMetadata.contains(metadata.Handle))
		{
			AssetMetaData& md = m_LoadedAssetMetadata.at(metadata.Handle);
			md = metadata;
		}
	}

	void EditorAssetThread::OnAssetLoaded(const AssetMetaData& metadata)
	{
		if (metadata.IsMemoryAsset)
			return;

		std::scoped_lock lock(m_LoadedAssetMetadataMutex);
		m_LoadedAssetMetadata[metadata.Handle] = metadata;
	}

	void EditorAssetThread::AssetThreadFunc()
	{
		SK_PROFILE_FRAME("AssetThread");

		while (m_Running)
		{
			if (m_LoadingQueue.empty())
			{
				m_IdleSignal.Set();

				SK_PROFILE_SCOPED("Idle");
				std::unique_lock lock(m_WorkAvailableMutex);
				if (m_MonitorAssets)
					m_WorkAvailable.wait_for(lock, m_MonitorAssetsIntervall);
				else
					m_WorkAvailable.wait(lock);

				m_IdleSignal.Reset();
			}

			SK_PROFILE_SCOPED("Busy");
			//SK_LOG_IF(loadQueue.size() > 0, LoggerType::Core, LogLevel::Trace, "AssetThread", "Loading {} Assets", loadQueue.size());

			while (true)
			{
				std::unique_lock lock(m_ALRStorageMutex);
				if (m_LoadingQueue.empty())
					break;

				AssetHandle next = m_LoadingQueue.front();
				m_LoadingQueue.pop();

				AssetLoadRequest& alr = m_ALRStorage.at(next);
				lock.unlock();

				LoadAsset(alr);
			}

			if (m_MonitorAssets)
			{
				SK_PROFILE_SCOPED("Monitor Assets");

				std::unique_lock lock(m_LoadedAssetMetadataMutex);
				for (auto& [handle, metadata] : m_LoadedAssetMetadata)
				{
					EnsureCurrent(metadata);
				}
			}
		}

		// Just in case
		m_IdleSignal.Set();
	}

	void EditorAssetThread::LoadAsset(AssetLoadRequest& request)
	{
		SK_CORE_VERIFY(request.Metadata.IsMemoryAsset == false);
		SK_CORE_VERIFY((request.Metadata.Status == AssetStatus::Ready && !request.Reload) == false);

		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("AssetThread", "Loading {} {} {}", request.Metadata.Type, request.Metadata.Handle, request.Metadata.FilePath);

		bool success = AssetSerializer::TryLoadAsset(request.Asset, request.Metadata);

		if (!success)
		{
			std::scoped_lock lock(m_ALRStorageMutex);
			// TODO(moro): request.Future.SetError();

			SK_CORE_VERIFY(false);
			request.Metadata.Status = AssetStatus::Unloaded;

			return;
		}

		if (success)
		{
			auto absolutePath = GetFilesystemPath(request.Metadata);
			request.Metadata.LastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
			request.Metadata.Status = AssetStatus::Ready;

			{
				std::scoped_lock lock(m_LoadedAssetMetadataMutex);
				m_LoadedAssetMetadata[request.Metadata.Handle] = request.Metadata;
			}

			request.Future.Set(request.Asset);
			request.Future.Signal(true, false);
		}

		SK_CORE_INFO_TAG("AssetThread", "Finished loading {} {} {}", request.Metadata.Type, request.Metadata.Handle, request.Metadata.FilePath);

		// NOTE(moro): After this next scope accessing the current alr is no longer save
		//             As soon as the handle is added to m_LoadedRequests the alr can be deleted from m_ALRStorage by the main thread
		{
			std::scoped_lock lock(m_ALRStorageMutex);
			m_HandledRequests.emplace_back(request.Metadata.Handle);
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
			return FileSystem::Absolute(metadata.FilePath);

		return (Project::GetActiveAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

}
