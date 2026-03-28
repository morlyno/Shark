#pragma once

#include "Shark/Core/Threading.h"
#include "Shark/Asset/AssetMetadata.h"

#include "Shark/Asset/AssetManager/AssetManagerBase.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"
#include "Shark/Asset/AssetThread/AssetLoadContext.h"

#include <future>
#include <barrier>

namespace Shark {

	class ProjectConfig;
	class EditorAssetManager;

	struct AssetThreadSettings
	{
		bool MonitorAssets = true;
	};

	class EditorAssetThread : public RefCount
	{
	public:
		EditorAssetThread(Ref<ProjectConfig> project, const AssetThreadSettings& settings);
		~EditorAssetThread();

		void Stop();
		void RunTasks();
		void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets, std::vector<Ref<Asset>>& outPendingAssets);

		void QueueAssetLoad(AssetLoadRequest& alr);
		Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle);

		std::atomic<uint32_t>& GetOperations() { return m_MainThreadOperations; }
		void DebugLogTimes();

	private:
		void AssetThreadFunc(std::stop_token stopToken);
		void LoadAsset(AssetLoadRequest& request);
		bool FinishLoad(AssetLoadContext& context);

		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	private:
		Weak<ProjectConfig> m_Project;
		std::jthread m_Thread;

		std::condition_variable m_WorkAvailable;

		std::mutex m_Mutex;
		std::map<AssetHandle, AssetLoadRequest> m_RequestStorage;
		std::map<AssetHandle, AssetLoadContext> m_ContextStorage;
		std::map<AssetHandle, std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point>> m_RequestStartedAt;

		std::queue<AssetHandle> m_LoadingQueue;
		std::vector<AssetHandle> m_HandledRequests;
		std::vector<Ref<Asset>> m_PendingAssets;

		std::atomic<uint32_t> m_MainThreadOperations = 0;
		std::queue<AssetLoadContext*> m_PendingQueue;
		std::queue<AssetLoadContext*> m_FinalizeQueue;

		std::queue<AssetLoadContext*> m_WorkQueue;
	};

}
