#pragma once

#include "Shark/Core/Thread.h"
#include "Shark/Asset/AssetThread/AssetThreadBase.h"

namespace Shark {

	class EditorAssetThread : public AssetThreadBase
	{
	public:
		EditorAssetThread();
		~EditorAssetThread();

		virtual void QueueAssetLoad(const AssetLoadRequest& alr) override;
		virtual Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle) override;

		virtual void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets) override;
		virtual void UpdateLoadedAssetsMetadata(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets) override;

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
