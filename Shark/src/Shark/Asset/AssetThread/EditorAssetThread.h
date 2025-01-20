#pragma once

#include "Shark/Core/Threading.h"
#include "Shark/Asset/AssetMetadata.h"

#include "Shark/Asset/AssetManager/AssetManagerBase.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"

namespace Shark {

	struct AssetThreadSettings
	{
		bool MonitorAssets = true;
	};

	class EditorAssetThread : public RefCount
	{
	public:
		EditorAssetThread(const AssetThreadSettings& settings);
		~EditorAssetThread();

		void Stop();
		void WaitUntilIdle();
		void ForceSleep(bool sleep = true) { m_SleepRequested = true; }

		void QueueAssetLoad(AssetLoadRequest& alr);
		Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle);

		void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets);
		void UpdateLastWriteTime(AssetHandle handle, uint64_t lastWriteTime);
		void OnMetadataChanged(const AssetMetaData& metadata);
		void OnAssetLoaded(const AssetMetaData& metadata);

	private:
		void AssetThreadFunc();
		void LoadAsset(AssetLoadRequest& request);
		bool EnsureCurrent(AssetMetaData& metadata);

		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	private:
		Threading::Thread m_Thread;
		Threading::ThreadSignal m_IdleSignal;
		std::atomic<bool> m_SleepRequested = false;
		bool m_Running = true;

		bool m_MonitorAssets = false;
		std::chrono::seconds m_MonitorAssetsIntervall = 1s;

		std::mutex m_WorkAvailableMutex;
		std::condition_variable m_WorkAvailable;

		std::mutex m_ALRStorageMutex;
		std::map<AssetHandle, AssetLoadRequest> m_ALRStorage;

		std::queue<AssetHandle> m_LoadingQueue;
		std::vector<AssetHandle> m_HandledRequests;

		Threading::TrackedMutex m_LoadedAssetMetadataMutex = SKLockableInit("LoadedAssetMetadata");
		//std::mutex m_LoadedAssetMetadataMutex;
		std::unordered_map<AssetHandle, AssetMetaData> m_LoadedAssetMetadata;

	};

}
