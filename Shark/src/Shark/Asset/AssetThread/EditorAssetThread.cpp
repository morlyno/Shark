#include "skpch.h"
#include "EditorAssetThread.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Asset/AssetThread/AssetLoadContext.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

#include <ranges>

namespace Shark {

	EditorAssetThread::EditorAssetThread(Ref<ProjectConfig> project, const AssetThreadSettings& settings)
		: m_Project(project)
	{
		m_Thread = std::jthread(std::bind_front(&EditorAssetThread::AssetThreadFunc, this));
		Platform::SetThreadName(m_Thread, "AssetThread");
	}

	EditorAssetThread::~EditorAssetThread()
	{
		Stop();
	}

	void EditorAssetThread::Stop()
	{
		if (!m_Thread.joinable())
			return;

		m_Thread.request_stop();
		m_Thread.join();
		SK_CORE_WARN_TAG("AssetThread", "Thread Stopped");
	}

	void EditorAssetThread::RunTasks()
	{
		SK_PROFILE_FUNCTION();

		{
			std::scoped_lock lock(m_Mutex);
			SK_CORE_VERIFY(m_WorkQueue.empty());
			std::swap(m_WorkQueue, m_PendingQueue);
		}

		AssetLoadContext* context = nullptr;

		while (true)
		{
			{
				std::scoped_lock lock(m_Mutex);
				if (m_WorkQueue.empty())
					break;

				context = m_WorkQueue.front();
				m_WorkQueue.pop();
			}

			auto tasks = context->GetTasks();
			for (auto& task : tasks)
			{
				if (context->HasErrors())
					break;

				task(context);
			}

			std::scoped_lock lock(m_Mutex);
			if (context->Loading())
			{
				m_PendingQueue.push(context);
				continue;
			}
			
			m_FinalizeQueue.push(context);
			m_MainThreadOperations.fetch_sub(1);
			m_MainThreadOperations.notify_all();
			m_WorkAvailable.notify_one();
		}

		SK_CORE_VERIFY(m_WorkQueue.empty());
	}

	void EditorAssetThread::RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets, std::vector<Ref<Asset>>& outPendingAssets)
	{
		SK_PROFILE_FUNCTION();

		std::scoped_lock lock(m_Mutex);

		if (!m_PendingAssets.empty())
		{
			outPendingAssets = std::move(m_PendingAssets);
		}

		if (m_HandledRequests.empty())
			return;

		for (AssetHandle handle : m_HandledRequests)
		{
			AssetLoadRequest& request = m_RequestStorage.at(handle);
			//request.Future.Set(request.Asset);
			//request.Future.Signal();

			outLoadedAssets.push_back(request);
			m_RequestStorage.erase(handle);
			m_ContextStorage.erase(handle);
		}

		m_HandledRequests.clear();
	}

	void EditorAssetThread::QueueAssetLoad(AssetLoadRequest& alr)
	{
		SK_CORE_TRACE_TAG("AssetThread", "QueueAssetLoad - {} {}", alr.Metadata.Handle, alr.Metadata.FilePath);

		{
			std::scoped_lock lock(m_Mutex);
			SK_CORE_VERIFY(!m_RequestStorage.contains(alr.Metadata.Handle));

			m_RequestStorage[alr.Metadata.Handle] = alr;
			m_LoadingQueue.push(alr.Metadata.Handle);
		}

		m_WorkAvailable.notify_one();
	}

	Threading::Future<Ref<Asset>> EditorAssetThread::GetFuture(AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		if (!m_RequestStorage.contains(handle))
			return {};

		AssetLoadRequest& request = m_RequestStorage.at(handle);
		return request.Future;
	}

	void EditorAssetThread::DebugLogTimes()
	{
		std::scoped_lock lock(m_Mutex);

		auto now = std::chrono::system_clock::now();

		for (auto& [handle, time] : m_RequestStartedAt)
		{
			if (!m_RequestStorage.contains(handle))
				continue;

			if ((time.second + 30s) < now)
			{
				auto& request = m_RequestStorage.at(handle);
				std::chrono::duration totalTime = now - time.first;
				SK_CORE_WARN_TAG("AssetThread", "Asset {} '{}' is loading for {:%T}", request.Metadata.Handle, request.Metadata.FilePath, totalTime);
				time.second = now;
			}
		}

	}

	void EditorAssetThread::AssetThreadFunc(std::stop_token stopToken)
	{
		SK_PROFILE_FRAME("AssetThread");
		std::stop_callback guard{ stopToken, [this]
		{
			SK_CORE_WARN_TAG("AssetThread", "Stopping Thread");
			m_WorkAvailable.notify_all();
		}};

		while (!stopToken.stop_requested())
		{
			{
				std::unique_lock lock(m_Mutex);
				m_WorkAvailable.wait(lock, [&]() { return stopToken.stop_requested() || !m_LoadingQueue.empty() || !m_FinalizeQueue.empty(); });
			}

			SK_PROFILE_SCOPED("Busy");

			while (!stopToken.stop_requested())
			{
				std::unique_lock lock(m_Mutex);
				if (m_LoadingQueue.empty())
					break;

				AssetLoadRequest& alr = m_RequestStorage.at(m_LoadingQueue.front());
				m_RequestStartedAt[alr.Metadata.Handle] = [](auto time) { return std::pair{ time, time }; }(std::chrono::system_clock::now());
				m_LoadingQueue.pop();
				lock.unlock();

				LoadAsset(alr);
			}

			while (!stopToken.stop_requested())
			{
				std::unique_lock lock(m_Mutex);
				if (m_FinalizeQueue.empty())
					break;

				AssetLoadContext* context = m_FinalizeQueue.front();
				m_FinalizeQueue.pop();
				lock.unlock();

				FinishLoad(*context);
			}

		}

		if (!m_LoadingQueue.empty())
		{
			SK_CORE_WARN_TAG("AssetThread", "Skipping {} load requests after stop was requested", m_LoadingQueue.size());
		}

	}

	void EditorAssetThread::LoadAsset(AssetLoadRequest& request)
	{
		SK_CORE_VERIFY(request.Metadata.IsMemoryAsset == false);
		SK_CORE_VERIFY((request.Metadata.Status == AssetStatus::Ready && !request.Reload) == false);

		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("AssetThread", "Loading {} {} {}", request.Metadata.Type, request.Metadata.Handle, request.Metadata.FilePath);

		AssetLoadContext context(request.Metadata.Handle);
		const bool sucess = AssetSerializer::TryLoadAsset(request.Asset, request.Metadata, &context);
		context.FixStatus(sucess);

		if (context.Loading())
		{
			std::scoped_lock lock(m_Mutex);
			m_ContextStorage.emplace(request.Metadata.Handle, std::move(context));
			m_PendingQueue.push(&m_ContextStorage.at(request.Metadata.Handle));
			m_MainThreadOperations.fetch_add(1);
			m_MainThreadOperations.notify_all();
			return;
		}

		FinishLoad(context);
	}

	bool EditorAssetThread::FinishLoad(AssetLoadContext& context)
	{
		SK_CORE_ASSERT(!context.Loading());
		if (context.Loading())
			return false;

		auto& request = m_RequestStorage.at(context.GetAssetHandle());
		if (context.HasErrors())
		{
			SK_CORE_ERROR_TAG("AssetThread", "Failed to load asset {} '{}'{}", request.Metadata.Handle, request.Metadata.FilePath, fmt::join(context.GetErrors(), "\n - {}"));
			request.Metadata.Status = AssetStatus::Unloaded;

			// #TODO #async error handling + remove alr

			return true;
		}
		
		auto absolutePath = GetFilesystemPath(request.Metadata);
		request.Metadata.LastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		request.Metadata.Status = AssetStatus::Ready;

		SK_CORE_INFO_TAG("AssetThread", "Finished loading {} {} {}", request.Metadata.Type, request.Metadata.Handle, request.Metadata.FilePath);
		
		// NOTE(moro): After this accessing the current alr is no longer save
		//             As soon as the handle is added to m_LoadedRequests the alr can be deleted from m_ALRStorage by the main thread
		{
			std::scoped_lock lock(m_Mutex);

			auto assets = context.GetAssets() | std::views::values;
			m_PendingAssets.insert(m_PendingAssets.end(), assets.begin(), assets.end());
			m_HandledRequests.emplace_back(request.Metadata.Handle);
			m_MainThreadOperations.fetch_add(1);
			m_MainThreadOperations.notify_all();
			m_RequestStartedAt.erase(request.Metadata.Handle);
		}

		return true;
	}

	std::filesystem::path EditorAssetThread::GetFilesystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::Absolute(metadata.FilePath);

		return (m_Project.GetRef()->GetAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

}
