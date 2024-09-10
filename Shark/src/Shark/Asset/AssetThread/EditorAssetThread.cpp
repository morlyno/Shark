#include "skpch.h"
#include "EditorAssetThread.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"
#include <ranges>

namespace Shark {

	EditorAssetThread::EditorAssetThread(const AssetThreadSettings& settings)
		: m_Thread("AssetThread")
	{
		SK_CORE_VERIFY(settings.DefaultDependencyPolicy != LoadDependencyPolicy::Default, "LoadDependencyPolicy::Default is not allowed as the default policy.");

		m_MonitorAssets = settings.MonitorAssets;
		m_DefaultDependencyPolicy = settings.DefaultDependencyPolicy;

		m_IdleSignal = Threading::Signal(true, false);
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

	LoadDependencyPolicy EditorAssetThread::GetDependencyPolicy(AssetHandle handle)
	{
		std::scoped_lock lock(m_ALRStorageMutex);
		if (!m_ALRStorage.contains(handle))
			return m_DefaultDependencyPolicy;

		AssetLoadRequest& request = m_ALRStorage.at(handle);
		if (request.DependencyPolicy == LoadDependencyPolicy::Default)
			return m_DefaultDependencyPolicy;

		return request.DependencyPolicy;
	}

	void EditorAssetThread::HandleMetadataRequests(const AssetRegistry& registry)
	{
		SK_PROFILE_FUNCTION();

		std::scoped_lock lock(m_RequestedMetadataMutex);
		for (AssetHandle handle : m_MetadataRequests)
		{
			if (!registry.Contains(handle))
				continue;

			m_RequestedMetadata[handle] = registry.Read(handle);
		}
		m_MetadataRequests.clear();
		m_RequestsCompleted.notify_all();
	}

	void EditorAssetThread::RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets)
	{
		SK_PROFILE_FUNCTION();

		std::scoped_lock lock(m_ALRStorageMutex);
		if (m_LoadedRequests.empty())
			return;

		for (AssetHandle handle : m_LoadedRequests)
		{
			AssetLoadRequest& request = m_ALRStorage.at(handle);
			request.Future.Signal();

			outLoadedAssets.push_back(request);
			m_ALRStorage.erase(handle);
		}

		m_LoadedRequests.clear();
	}

	void EditorAssetThread::AddLoadedAsset(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		std::scoped_lock lock(m_LoadedAssetsMutex);
		m_LoadedAssets[metadata.Handle] = asset;

		// NOTE(moro): memory assets don't need to be stored because they are not monitored
		//             and they are always in up to date state in the registry
		//if (!metadata.IsMemoryAsset)
		{
			std::scoped_lock lock(m_LoadedAssetMetadataMutex);
			m_LoadedAssetMetadata[metadata.Handle] = metadata;
		}
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

				if (alr.DependencyPolicy == LoadDependencyPolicy::Default)
					alr.DependencyPolicy = m_DefaultDependencyPolicy;

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
		if (request.Metadata.IsMemoryAsset || (request.Metadata.Status == AssetStatus::Ready && !request.Reload))
		{
			if (!request.Asset)
			{
				std::scoped_lock lock(m_LoadedAssetsMutex);
				request.Asset = m_LoadedAssets.at(request.Metadata.Handle);
			}

			LoadDependencies(request);
			return;
		}

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

			{
				std::scoped_lock lock(m_LoadedAssetsMutex, m_LoadedAssetMetadataMutex);
				m_LoadedAssets[request.Metadata.Handle] = request.Asset;
				m_LoadedAssetMetadata[request.Metadata.Handle] = request.Metadata;

				m_LoadedAssetMetadata.at(request.Metadata.Handle).Status = AssetStatus::Ready;
			}

			LoadDependencies(request);
			request.Metadata.Status = AssetStatus::Ready;

			request.Future.Set(request.Asset);
			request.Future.Signal(true, false);
		}

		SK_CORE_INFO_TAG("AssetThread", "Finished loading {} {} {}", request.Metadata.Type, request.Metadata.Handle, request.Metadata.FilePath);

		// NOTE(moro): After this next scope accessing the current alr is no longer save
		//             As soon as the handle is added to m_LoadedRequests the alr can be deleted from m_ALRStorage by the main thread
		{
			std::scoped_lock lock(m_ALRStorageMutex);
			m_LoadedRequests.emplace_back(request.Metadata.Handle);
		}
	}

	void EditorAssetThread::LoadDependencies(const AssetLoadRequest& request)
	{
		SK_CORE_VERIFY(request.Asset);
		SK_CORE_VERIFY(request.DependencyPolicy != LoadDependencyPolicy::Default);
		if (request.DependencyPolicy == LoadDependencyPolicy::OnDemand || !request.Asset)
			return;

		SK_PROFILE_FUNCTION();

		std::vector<AssetMetaData> dependencies;
		std::vector<AssetHandle> deferredDependencies;

		const auto Queue = [this, &dependencies, &deferredDependencies, dependencyPolicy = request.DependencyPolicy, reload = request.Reload](AssetHandle handle)
		{
			if (!handle)
				return;

			{
				// early out if already queued
				std::scoped_lock lock(m_ALRStorageMutex);
				if (m_ALRStorage.contains(handle))
				{
					AssetLoadRequest& alr = m_ALRStorage.at(handle);
					if (alr.Metadata.Status == AssetStatus::Loading)
					{
						if (alr.DependencyPolicy < dependencyPolicy)
							alr.DependencyPolicy = dependencyPolicy;
						return;
					}

					if (alr.Metadata.Status == AssetStatus::Ready)
						return;

					SK_DEBUG_BREAK_CONDITIONAL(s_Break_Unreachable);
				}
			}

			{
				std::scoped_lock lock(m_LoadedAssetMetadataMutex);
				if (m_LoadedAssetMetadata.contains(handle) && std::ranges::find(dependencies, handle, &AssetMetaData::Handle) != dependencies.end())
				{
					dependencies.push_back(m_LoadedAssetMetadata.at(handle));
					return;
				}
			}

			deferredDependencies.push_back(handle);
		};

		switch (request.Metadata.Type)
		{
			case AssetType::Scene:
			{
				Ref<Scene> scene = request.Asset.As<Scene>();

				for (auto entities = scene->GetAllEntitysWith<MeshComponent>();
					 auto ent : entities)
				{
					Entity entity(ent, scene);
					auto& component = entity.GetComponent<MeshComponent>();
					Queue(component.Mesh);
					Queue(component.Material);
				}

				for (auto entities = scene->GetAllEntitysWith<SkyComponent>();
					 auto ent : entities)
				{
					Entity entity(ent, scene);
					auto& component = entity.GetComponent<SkyComponent>();
					Queue(component.SceneEnvironment);
				}

				for (auto entities = scene->GetAllEntitysWith<SpriteRendererComponent>();
					 auto ent : entities)
				{
					Entity entity(ent, scene);
					auto& component = entity.GetComponent<SpriteRendererComponent>();
					Queue(component.TextureHandle);
				}

				for (auto entities = scene->GetAllEntitysWith<TextRendererComponent>();
					 auto ent : entities)
				{
					Entity entity(ent, scene);
					auto& component = entity.GetComponent<TextRendererComponent>();
					Queue(component.FontHandle);
				}

				break;
			}
			case AssetType::MeshSource:
			{
				Ref<MeshSource> meshSource = request.Asset.As<MeshSource>();
				for (AssetHandle material : meshSource->GetMaterials())
					Queue(material);
				break;
			}
			case AssetType::Mesh:
			{
				Ref<Mesh> mesh = request.Asset.As<Mesh>();
				Queue(mesh->GetMeshSource());
				break;
			}
			case AssetType::Material:
			{
				Ref<MaterialAsset> material = request.Asset.As<MaterialAsset>();
				Queue(material->GetAlbedoMap());
				Queue(material->GetNormalMap());
				Queue(material->GetMetalnessMap());
				Queue(material->GetRoughnessMap());
				break;
			}
		}

		{
			std::scoped_lock lock(m_RequestedMetadataMutex);
			for (AssetHandle handle : deferredDependencies)
			{
				m_MetadataRequests.emplace(handle);
			}
		}

		const auto Load = [this](AssetMetaData& metadata, const AssetLoadRequest& parentRequest)
		{
			metadata.Status = AssetStatus::Loading;
			AssetLoadRequest alr(metadata);
			alr.DependencyPolicy = parentRequest.DependencyPolicy;
			alr.Reload = parentRequest.Reload;

			switch (alr.DependencyPolicy)
			{
				case LoadDependencyPolicy::Deferred:
				{
					QueueAssetLoad(alr);
					break;
				}
				case LoadDependencyPolicy::Immediate:
				{
					m_ALRStorageMutex.lock();
					SK_CORE_VERIFY(m_ALRStorage.contains(metadata.Handle) == false);
					m_ALRStorage[metadata.Handle] = std::move(alr);

					AssetLoadRequest& activeALR = m_ALRStorage.at(metadata.Handle);
					m_ALRStorageMutex.unlock();

					LoadAsset(activeALR);
					break;
				}
				default:
				{
					SK_CORE_VERIFY(false, "Invalid LoadDependencyPolicy");
					break;
				}
			}
		};

		for (auto& metadata : dependencies)
		{
			Load(metadata, request);
		}

		if (!deferredDependencies.empty())
		{
			SK_PROFILE_SCOPED("Load Deferred Dependencies");

			for (AssetHandle handle : deferredDependencies)
			{
				std::optional<AssetMetaData> metadata;
				{
					std::unique_lock lock(m_RequestedMetadataMutex);
					if (!m_RequestedMetadata.contains(handle))
					{
						if (!m_MetadataRequests.contains(handle))
						{
							SK_CORE_ERROR_TAG("AssetThread", "Failed to obtain metadata for {}", handle);
							break;
						}

						SK_PROFILE_SCOPED("Wait for Reqeusts");
						m_RequestsCompleted.wait(lock, [this, handle]() mutable
						{
							if (m_RequestedMetadata.contains(handle))
								return true;
							if (!m_MetadataRequests.contains(handle))
								return true; // error, continue
							return false;
						});
					}

					if (m_RequestedMetadata.contains(handle))
					{
						metadata = m_RequestedMetadata.at(handle);
						m_RequestedMetadata.erase(handle);
					}
				}

				if (metadata)
				{
					Load(*metadata, request);
				}
			}
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
