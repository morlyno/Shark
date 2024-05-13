#pragma once

#include "Shark/Core/Thread.h"
#include "Shark/Asset/AssetMetadata.h"

#include "Shark/Asset/AssetManager/AssetManagerBase.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"

namespace Shark {

	class EditorAssetThread : public RefCount
	{
	public:
		EditorAssetThread();
		~EditorAssetThread();

		void Stop();

		void QueueAssetLoad(const AssetLoadRequest& alr);
		Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle);
		AssetMetaData GetLoadedAssetMetadata(AssetHandle handle);

		void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets);
		void UpdateLoadedAssetsMetadata(const LoadedAssetsMap& loadedAssets, const AssetRegistry& registry);

	private:
		void AssetThreadFunc();
		void MonitorAssets();
		bool EnsureCurrent(AssetMetaData& metadata);

		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	private:
		Thread m_Thread;
		bool m_Running = true;

		bool m_MonitorAssets = true;
		std::chrono::seconds m_MonitorAssetsIntervall = 1s;

		std::mutex m_WorkAvailableMutex;
		std::condition_variable m_WorkAvailable;

		std::mutex m_LoadingQueueMutex;
		std::vector<AssetLoadRequest> m_LoadingQueue;

		std::mutex m_LoadedAssetsMutex;
		std::vector<AssetLoadRequest> m_LoadedAssets;

		std::mutex m_LoadedAssetMetadataMutex;
		std::vector<AssetMetaData> m_LoadedAssetMetadata;
	};

}
