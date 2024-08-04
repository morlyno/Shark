#pragma once

#include "Shark/Core/Threading.h"
#include "Shark/Asset/AssetMetadata.h"

#include "Shark/Asset/AssetManager/AssetManagerBase.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"

namespace Shark {

	struct AssetThreadSettings
	{
		bool MonitorAssets = true;
		LoadDependencyPolicy DefaultDependencyPolicy = LoadDependencyPolicy::OnDemand;
		const AssetRegistry* Registry;
	};

	class EditorAssetThread : public RefCount
	{
	public:
		EditorAssetThread(const AssetThreadSettings& settings);
		~EditorAssetThread();

		void Stop();
		void WaitUntilIdle();

		void QueueAssetLoad(AssetLoadRequest& alr);
		Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle);
		LoadDependencyPolicy GetDependencyPolicy(AssetHandle handle);
		LoadDependencyPolicy GetDefaultDependencyPolicy() const { return m_DefaultDependencyPolicy; }

		void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets);
		void AddLoadedAsset(Ref<Asset> asset, const AssetMetaData& metadata);

	private:
		void AssetThreadFunc();
		void LoadAsset(AssetLoadRequest& request);
		void LoadDependencies(const AssetLoadRequest& request);
		bool EnsureCurrent(AssetMetaData& metadata);

		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	private:
		Thread m_Thread;
		Threading::Signal m_IdleSignal = { UninitializedTag };
		bool m_Running = true;

		LoadDependencyPolicy m_DefaultDependencyPolicy;
		bool m_MonitorAssets = false;
		std::chrono::seconds m_MonitorAssetsIntervall = 1s;

		std::mutex m_WorkAvailableMutex;
		std::condition_variable m_WorkAvailable;

		std::mutex m_ALRStorageMutex;
		std::map<AssetHandle, AssetLoadRequest> m_ALRStorage;

		std::queue<AssetHandle> m_LoadingQueue;
		std::vector<AssetHandle> m_LoadedRequests;

		std::mutex m_LoadedAssetMetadataMutex;
		std::unordered_map<AssetHandle, AssetMetaData> m_LoadedAssetMetadata;

		std::mutex m_LoadedAssetsMutex;
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

		const AssetRegistry* m_Registry;

	};

}
