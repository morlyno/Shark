#include "skpch.h"
#include "EditorAssetManager.h"

#include "Shark/Core/Project.h"
#include "Shark/Event/ApplicationEvent.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/Asset/AssetThread/EditorAssetThread.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	namespace utils {
		
		static std::filesystem::path GetFilesystemKey(const std::filesystem::path& path)
		{
			return std::filesystem::absolute(path).lexically_normal();
		}

	}

	static AssetMetaData s_NullMetadata;

	static std::map<AssetType, Ref<Asset>(*)()> s_Placeholders = {
		{ AssetType::Environment, []() -> Ref<Asset> { return Renderer::GetEmptyEnvironment(); } },
		{ AssetType::Texture, []() -> Ref<Asset> { return Renderer::GetWhiteTexture(); } }
	};

	EditorAssetManager::EditorAssetManager(Ref<Project> project)
		: m_Project(project)
	{
		AssetSerializer::RegisterSerializers();
		ReadImportedAssetsFromDisc();

		AssetThreadSettings settings;
		settings.MonitorAssets = true;
		m_AssetThread = Ref<EditorAssetThread>::Create(settings);
	}

	EditorAssetManager::~EditorAssetManager()
	{
		m_AssetThread->Stop();

		WriteImportedAssetsToDisc();
		AssetSerializer::ReleaseSerializers();
	}

	void EditorAssetManager::SerializeImportedAssets()
	{
		WriteImportedAssetsToDisc();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle)
	{
		if (!m_Registry.Contains(handle))
			return AssetType::None;

		const auto& metadata = m_Registry.Read(handle);
		return metadata.Type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		if (!IsValidAssetHandle(handle))
			return nullptr;

		if (IsMemoryAsset(handle))
			return m_MemoryOnlyAssets.at(handle);

		AssetMetaData& metadata = m_Registry.Get(handle);
		if (metadata.Status != AssetStatus::Ready)
		{
			if (metadata.Status == AssetStatus::Loading)
			{
				Threading::Future future = m_AssetThread->GetFuture(handle);
				return future.WaitAndGet();
			}

			Ref<Asset> asset = nullptr;
			bool success = AssetSerializer::TryLoadAsset(asset, metadata);
			if (success)
			{
				m_LoadedAssets[handle] = asset;
				metadata.LastWriteTime = FileSystem::GetLastWriteTime(GetFilesystemPath(metadata));
				metadata.Status = AssetStatus::Ready;
				m_AssetThread->OnAssetLoaded(metadata);
			}
			return asset;
		}

		return m_LoadedAssets.at(handle);
	}

	AsyncLoadResult<Asset> EditorAssetManager::GetAssetAsync(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		if (!IsValidAssetHandle(handle))
			return { nullptr, false };

		if (IsMemoryAsset(handle))
			return { m_MemoryOnlyAssets.at(handle), true };

		AssetMetaData& metadata = m_Registry.Get(handle);
		if (metadata.Status != AssetStatus::Ready)
		{
			if (metadata.Status != AssetStatus::Loading)
			{
				metadata.Status = AssetStatus::Loading;
				AssetLoadRequest alr(metadata);
				m_AssetThread->QueueAssetLoad(alr);
			}

			return { GetPlaceholder(metadata.Type), false };
		}

		return { m_LoadedAssets.at(handle), true };
	}

	Threading::Future<Ref<Asset>> EditorAssetManager::GetAssetFuture(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		if (!IsValidAssetHandle(handle))
			return {};

		if (IsMemoryAsset(handle))
			return Threading::Future(m_LoadedAssets.at(handle));

		AssetMetaData& metadata = m_Registry.Get(handle);
		if (metadata.Status != AssetStatus::Ready)
		{
			if (metadata.Status != AssetStatus::Loading)
			{
				metadata.Status = AssetStatus::Loading;
				AssetLoadRequest alr(metadata);
				m_AssetThread->QueueAssetLoad(alr);
				return alr.Future;
			}

			return m_AssetThread->GetFuture(handle);
		}

		return Threading::Future(m_LoadedAssets.at(handle));
	}

	std::vector<AssetHandle> EditorAssetManager::GetAllAssetsOfType(AssetType assetType)
	{
		std::vector<AssetHandle> assets;

		// registry access read only
		for (const auto& [handle, metadata] : m_Registry)
		{
			if (metadata.Type == assetType)
			{
				assets.push_back(handle);
			}
		}

		return assets;
	}

	AssetHandle EditorAssetManager::AddAsset(Ref<Asset> asset, const std::filesystem::path& filepath)
	{
		if (IsValidAssetHandle(asset->Handle))
		{
			SK_CORE_ERROR_TAG("AssetManager", "Tried adding an Asset with a valid AssetHandle!");
			return asset->Handle;
		}

		if (asset->Handle == AssetHandle::Invalid)
			asset->Handle = AssetHandle::Generate();

		AssetMetaData metadata;
		metadata.Handle = asset->Handle;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = MakeRelativePath(FileSystem::UniquePath(filepath));
		metadata.Status = AssetStatus::Ready;

		AssetSerializer::Serialize(asset, metadata);
		metadata.LastWriteTime = FileSystem::GetLastWriteTime(GetFilesystemPath(metadata));

		m_Registry.Add(metadata);
		m_LoadedAssets[metadata.Handle] = asset;
		WriteImportedAssetsToDisc();

		m_AssetThread->OnAssetLoaded(metadata);

		SK_CORE_WARN_TAG("AssetManager", "Asset Created (Type: {0}, Handle: 0x{1:x}, FilePath: {2}", metadata.Type, metadata.Handle, metadata.FilePath);
		return metadata.Handle;
	}

	AssetHandle EditorAssetManager::AddMemoryAsset(Ref<Asset> asset)
	{
		if (IsValidAssetHandle(asset->Handle))
		{
			SK_CORE_ERROR_TAG("AssetManager", "Tried adding a Memory Asset with a valid AssetHandle!");
			return asset->Handle;
		}

		AssetHandle newHandle = asset->Handle;
		if (newHandle == AssetHandle::Invalid)
			newHandle = AssetHandle::Generate();

		AssetMetaData metadata;
		metadata.Type = asset->GetAssetType();
		metadata.Handle = newHandle;
		metadata.IsMemoryAsset = true;
		metadata.Status = AssetStatus::Ready;
		m_Registry.Add(metadata);

		asset->Handle = metadata.Handle;
		m_MemoryOnlyAssets[metadata.Handle] = asset;

		m_AssetThread->OnAssetLoaded(metadata);

		SK_CORE_INFO_TAG("AssetManager", "Memory Asset Added (Type: {}, Handle: 0x{:x})", metadata.Type, metadata.Handle);
		return metadata.Handle;
	}

	bool EditorAssetManager::ReloadAsset(AssetHandle handle)
	{
		SK_NOT_IMPLEMENTED();
		return false;

#if 0
		SK_CORE_VERIFY(false);
		AssetMetaData& metadata = GetMetadataInternal(handle);
		if (!metadata.IsValid())
			return false;

		if (metadata.Status == AssetStatus::Loading)
		{
			Threading::Future future = m_AssetThread->GetFuture(handle);
			future.Wait();

			m_LoadedAssets[handle] = future.Get();
			AssetMetaData metadata = m_AssetThread->GetLoadedAssetMetadata(handle);
			if (metadata.Handle)
				m_Registry[handle] = metadata;

			return true;
		}

		Ref<Asset> asset;
		bool success = AssetSerializer::TryLoadAsset(asset, metadata);
		if (success)
		{
			m_LoadedAssets[handle] = asset;
			metadata.LastWriteTime = FileSystem::GetLastWriteTime(GetFilesystemPath(metadata));
			metadata.Status = AssetStatus::Ready;

			switch (metadata.Type)
			{
				case AssetType::Mesh:
				{
					Ref<Mesh> mesh = asset.As<Mesh>();
					ReloadAsset(mesh->GetMeshSource());
					break;
				}
				case AssetType::Material:
				{
					Ref<MaterialAsset> material = asset.As<MaterialAsset>();
					ReloadAsset(material->GetAlbedoMap());
					ReloadAsset(material->GetNormalMap());
					ReloadAsset(material->GetMetalnessMap());
					ReloadAsset(material->GetRoughnessMap());
					material->Invalidate();
					break;
				}
			}
		}

		return metadata.Status == AssetStatus::Ready;
#endif
	}

	void EditorAssetManager::ReloadAssetAsync(AssetHandle handle)
	{
		if (!IsValidAssetHandle(handle))
			return;

		const AssetMetaData& metadata = m_Registry.Read(handle);

		AssetLoadRequest alr(metadata, true);
		m_AssetThread->QueueAssetLoad(alr);
	}

	bool EditorAssetManager::DependenciesLoaded(AssetHandle handle, bool loadIfNotReady)
	{
		if (!IsValidAssetHandle(handle))
			return true;

		const AssetMetaData& metadata = m_Registry.Read(handle);

		if (metadata.Status == AssetStatus::Loading)
			return false;

		if (metadata.Status != AssetStatus::Ready)
		{
			if (loadIfNotReady)
				GetAssetAsync(handle);

			return false;
		}

		Ref<Asset> asset = GetAsset(handle);
		switch (metadata.Type)
		{
			case AssetType::Scene:
			{
				Ref<Scene> scene = asset.As<Scene>();
				{
					auto entities = scene->GetAllEntitysWith<MeshComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& meshComp = entity.GetComponent<MeshComponent>();
						if (!DependenciesLoaded(meshComp.Mesh, loadIfNotReady))
							return false;
					}
				}
				
				{
					auto entities = scene->GetAllEntitysWith<SubmeshComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& meshComp = entity.GetComponent<SubmeshComponent>();
						if (!DependenciesLoaded(meshComp.Mesh, loadIfNotReady))
							return false;
						if (!DependenciesLoaded(meshComp.Material, loadIfNotReady))
							return false;
					}
				}

				{
					auto entities = scene->GetAllEntitysWith<StaticMeshComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& meshComp = entity.GetComponent<StaticMeshComponent>();
						if (!DependenciesLoaded(meshComp.StaticMesh, loadIfNotReady))
							return false;
						for (auto [index, material] : meshComp.MaterialTable->GetMaterials())
							if (!DependenciesLoaded(material, loadIfNotReady))
								return false;
					}
				}

				{
					auto entities = scene->GetAllEntitysWith<SkyComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& skyComp = entity.GetComponent<SkyComponent>();
						if (!DependenciesLoaded(skyComp.SceneEnvironment, loadIfNotReady))
							return false;
					}
				}

				{
					auto entities = scene->GetAllEntitysWith<SpriteRendererComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& src = entity.GetComponent<SpriteRendererComponent>();
						if (!DependenciesLoaded(src.TextureHandle, loadIfNotReady))
							return false;
					}
				}

				{
					auto entities = scene->GetAllEntitysWith<TextRendererComponent>();
					for (auto ent : entities)
					{
						Entity entity(ent, scene);
						auto& trc = entity.GetComponent<TextRendererComponent>();
						if (!DependenciesLoaded(trc.FontHandle, loadIfNotReady))
							return false;
					}
				}

				break;
			}
			case AssetType::Texture:
				break;
			case AssetType::MeshSource:
			{
				Ref<MeshSource> meshSource = asset.As<MeshSource>();
				for (auto& material : meshSource->GetMaterials())
				{
					if (!DependenciesLoaded(material, loadIfNotReady))
						return false;
				}
				break;
			}
			case AssetType::Mesh:
			{
				Ref<Mesh> mesh = asset.As<Mesh>();
				AssetHandle meshSource = mesh->GetMeshSource();
				if (meshSource && !DependenciesLoaded(mesh->GetMeshSource(), loadIfNotReady))
					return false;
				break;
			}
			case AssetType::Material:
			{
				Ref<MaterialAsset> material = asset.As<MaterialAsset>();
				if (!DependenciesLoaded(material->GetAlbedoMap(), loadIfNotReady))
					return false;
				if (!DependenciesLoaded(material->GetNormalMap(), loadIfNotReady))
					return false;
				if (!DependenciesLoaded(material->GetMetalnessMap(), loadIfNotReady))
					return false;
				if (!DependenciesLoaded(material->GetRoughnessMap(), loadIfNotReady))
					return false;
				break;
			}
			case AssetType::Environment:
				break;
		}

		return true;
	}

	bool EditorAssetManager::IsValidAssetHandle(AssetHandle handle)
	{
		return handle != AssetHandle::Invalid && m_Registry.Contains(handle);
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle handle) 
	{
		return m_MemoryOnlyAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle) || m_MemoryOnlyAssets.contains(handle);
	}

	void EditorAssetManager::DeleteAsset(AssetHandle handle)
	{
		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

		m_Registry.Remove(handle);

		WriteImportedAssetsToDisc();
	}

	void EditorAssetManager::DeleteMemoryAsset(AssetHandle handle)
	{
		if (IsMemoryAsset(handle))
		{
			m_LoadedAssets.erase(handle);
			m_Registry.Remove(handle);
		}
	}

	bool EditorAssetManager::SaveAsset(AssetHandle handle)
	{
		if (!IsValidAssetHandle(handle) || IsMemoryAsset(handle))
			return false;

		AssetMetaData& metadata = m_Registry.Get(handle);
		if (metadata.Status != AssetStatus::Ready)
			return false;

		if (AssetSerializer::Serialize(m_LoadedAssets.at(handle), metadata))
		{
			metadata.LastWriteTime = FileSystem::GetLastWriteTime(GetFilesystemPath(metadata));
			m_AssetThread->UpdateLastWriteTime(handle, metadata.LastWriteTime);
			return true;
		}

		return false;
	}

	void EditorAssetManager::WaitUntilIdle()
	{
		m_AssetThread->WaitUntilIdle();
	}

	void EditorAssetManager::SyncWithAssetThread()
	{
		std::vector<AssetLoadRequest> loadedAssets;
		m_AssetThread->RetrieveLoadedAssets(loadedAssets);

		for (AssetLoadRequest& alr : loadedAssets)
		{
			if (!m_Registry.Contains(alr.Metadata.Handle))
				continue; // file was deleted. nothing to do

			if (m_LoadedAssets.contains(alr.Metadata.Handle))
				m_LoadedAssets.at(alr.Metadata.Handle)->SetFlag(AssetFlag::Invalid, true);

			m_Registry.Update(alr.Metadata);
			m_LoadedAssets[alr.Metadata.Handle] = alr.Asset;

			if (alr.Reload)
			{
				Application::Get().DispatchEvent<AssetReloadedEvent>(alr.Metadata.Handle);
			}
		}
	}

	Ref<Asset> EditorAssetManager::GetPlaceholder(AssetType assetType)
	{
		if (s_Placeholders.contains(assetType))
			return s_Placeholders.at(assetType)();
		return nullptr;;
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(AssetHandle handle) const
	{
		if (m_Registry.Contains(handle))
			return m_Registry.Read(handle);
		return s_NullMetadata;
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(Ref<Asset> asset) const
	{
		return GetMetadata(asset->Handle);
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(const std::filesystem::path& filepath) const
	{
		AssetHandle handle = GetAssetHandleFromFilepath(filepath);
		return GetMetadata(handle);
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("EditorAssetManager::GetAssetHandleFromFilepath");
		auto relativePath = MakeRelativePath(filepath);
		return m_Registry.TryFind(relativePath);
	}

	bool EditorAssetManager::IsFileImported(const std::filesystem::path& filepath) const
	{
		AssetHandle handle = GetAssetHandleFromFilepath(filepath);
		return m_Registry.Contains(handle);
	}

	std::filesystem::path EditorAssetManager::GetFilesystemPath(AssetHandle handle) const
	{
		if (!m_Registry.Contains(handle))
			return {};

		const auto& metadata = m_Registry.Read(handle);
		return GetFilesystemPath(metadata);
	}

	std::filesystem::path EditorAssetManager::GetFilesystemPath(const AssetMetaData& metadata) const
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::Absolute(metadata.FilePath);

		SK_CORE_VERIFY(!m_Project.Expired());
		return (m_Project.GetRef()->GetAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

	std::filesystem::path EditorAssetManager::MakeRelativePath(const std::filesystem::path& filepath) const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return std::filesystem::relative(m_Project.GetRef()->GetAbsolute(filepath), m_Project.GetRef()->GetAssetsDirectory());
	}

	std::string EditorAssetManager::MakeRelativePathString(const std::filesystem::path& filepath) const
	{
		return MakeRelativePath(filepath).string();
	}

	std::filesystem::path EditorAssetManager::GetProjectPath(const AssetMetaData& metadata) const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return m_Project.GetRef()->GetRelative(GetFilesystemPath(metadata));
	}

	bool EditorAssetManager::HasExistingFilePath(const AssetMetaData& metadata)
	{
		auto filesystemPath = GetFilesystemPath(metadata);
		return FileSystem::Exists(filesystemPath);
	}

	bool EditorAssetManager::HasExistingFilePath(AssetHandle handle)
	{
		auto filesystemPath = GetFilesystemPath(handle);
		return FileSystem::Exists(filesystemPath);
	}

	bool EditorAssetManager::ImportMemoryAsset(AssetHandle handle, const std::string& directory, const std::string& filename)
	{
		SK_NOT_IMPLEMENTED();
		return false;

#if 0
		if (!IsMemoryAsset(handle))
			return false;

		auto& metadata = GetMetadataInternal(handle);
		metadata.FilePath = MakeRelativePath(directory + "/" + filename);
		metadata.IsMemoryAsset = false;

		if (std::filesystem::exists(metadata.FilePath))
		{
			uint32_t count = 1;
			bool foundValidFilePath = false;
			while (!foundValidFilePath)
			{
				metadata.FilePath = fmt::format("{}/{} ({:2})", directory, filename, count++);
				foundValidFilePath = !std::filesystem::exists(metadata.FilePath);
			}
		}

		Ref<Asset> asset = m_LoadedAssets.at(handle);
		if (!AssetSerializer::Serialize(asset, metadata))
		{
			metadata.FilePath.clear();
			metadata.IsMemoryAsset = true;
			return false;
		}

		WriteImportedAssetsToDisc();
		return true;
#endif
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		AssetType type = AssetUtils::GetAssetTypeFromPath(filepath);
		if (type == AssetType::None)
			return AssetHandle::Invalid;

		auto fsPath = FileSystem::Absolute(filepath);
		if (!FileSystem::Exists(fsPath))
			return AssetHandle::Invalid;

		AssetHandle handle = GetAssetHandleFromFilepath(fsPath);
		if (handle != AssetHandle::Invalid)
			return handle;

		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.FilePath = MakeRelativePath(fsPath);
		metadata.Type = type;
		metadata.Status = AssetStatus::Unloaded;
		m_Registry.Add(metadata);
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("AssetManager", "Imported Asset => Handle: 0x{:x}, Type: {}, FilePath: {}", metadata.Handle, metadata.Type, metadata.FilePath);
		return metadata.Handle;
	}

	const LoadedAssetsMap& EditorAssetManager::GetLoadedAssets() const
	{
		return m_LoadedAssets;
	}

	AssetRegistry& EditorAssetManager::GetAssetRegistry()
	{
		return m_Registry;
	}

	const AssetRegistry& EditorAssetManager::GetAssetRegistry() const
	{
		return m_Registry;
	}

	bool EditorAssetManager::AssetMoved(AssetHandle handle, const std::filesystem::path& newpath)
	{
		if (!IsValidAssetHandle(handle))
			return false;

		auto& metadata = m_Registry.Get(handle);
		metadata.FilePath = MakeRelativePath(newpath);
		WriteImportedAssetsToDisc();
		SK_CORE_WARN_TAG("AssetManager", "Filepath Changed => Handle: {}, Filepath: {}", handle, newpath);

		return true;
	}

	bool EditorAssetManager::AssetRenamed(AssetHandle handle, const std::string& newName)
	{
		if (!IsValidAssetHandle(handle))
			return false;

		AssetMetaData& metadata = m_Registry.Get(handle);
		FileSystem::ReplaceStem(metadata.FilePath, newName);
		WriteImportedAssetsToDisc();
		SK_CORE_WARN_TAG("AssetManager", "Filename Changed => Handle: {}, new path: {}", handle, metadata.FilePath);

		return true;
	}

	bool EditorAssetManager::AssetDeleted(AssetHandle handle)
	{
		if (!IsValidAssetHandle(handle))
			return false;

		SK_CORE_WARN_TAG("AssetManager", "Asset Deleted => Handle: {}", handle, GetMetadata(handle).FilePath);
		m_Registry.Remove(handle);
		WriteImportedAssetsToDisc();
		return true;
	}

	AssetHandle EditorAssetManager::GetEditorAsset(const std::filesystem::path& filepath)
	{
		SK_PROFILE_FUNCTION();
		// TODO(moro): handle non existing file
		SK_CORE_VERIFY(FileSystem::Exists(filepath));

		const auto key = utils::GetFilesystemKey(filepath);
		if (!m_EditorAssets.contains(key))
			m_EditorAssets[key] = AssetHandle::Generate();

		AssetHandle handle = m_EditorAssets.at(key);
		SK_CORE_VERIFY(handle != AssetHandle::Invalid);
		if (!m_Registry.Contains(handle))
		{
			AssetMetaData metadata;
			metadata.FilePath = filepath;
			metadata.Handle = handle;
			metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
			metadata.IsEditorAsset = true;
			m_Registry.Add(metadata);
			WriteImportedAssetsToDisc();
		}

		return handle;
	}

	AssetHandle EditorAssetManager::AddEditorAsset(AssetHandle handle, const std::filesystem::path& filepath)
	{
		const auto key = utils::GetFilesystemKey(filepath);
		if (m_EditorAssets.contains(key))
			return m_EditorAssets.at(key);

		m_EditorAssets[key] = handle;
		if (m_Registry.Contains(handle))
			return handle;

		AssetMetaData metadata;
		metadata.FilePath = filepath;
		metadata.Handle = handle;
		metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
		metadata.IsEditorAsset = true;
		m_Registry.Add(metadata);
		WriteImportedAssetsToDisc();
		return handle;
	}

	AssetHandle EditorAssetManager::AddEditorAsset(Ref<Asset> asset, const std::filesystem::path& filepath)
	{
		const auto key = utils::GetFilesystemKey(filepath);
		if (m_EditorAssets.contains(key))
			return m_EditorAssets.at(key);

		if (asset->Handle == AssetHandle::Invalid)
			asset->Handle = AssetHandle::Generate();

		m_EditorAssets[key] = asset->Handle;
		if (m_Registry.Contains(asset->Handle))
			return asset->Handle;

		AssetMetaData metadata;
		metadata.FilePath = filepath;
		metadata.Handle = asset->Handle;
		metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
		metadata.IsEditorAsset = true;
		metadata.Status = AssetStatus::Ready;
		m_Registry.Add(metadata);
		m_LoadedAssets[asset->Handle] = asset;
		WriteImportedAssetsToDisc();

		return asset->Handle;
	}

	bool EditorAssetManager::HasEditorAsset(const std::filesystem::path& filepath) const
	{
		const auto key = utils::GetFilesystemKey(filepath);
		return m_EditorAssets.contains(key);
	}

	struct ImportedAssetsEntry
	{
		AssetHandle Handle;
		AssetType Type;
		std::filesystem::path FilePath;

		ImportedAssetsEntry() = default;
		ImportedAssetsEntry(const AssetMetaData& metadata)
			: Handle(metadata.Handle), Type(metadata.Type), FilePath(metadata.FilePath) {}

		bool operator<(const ImportedAssetsEntry& rhs) const
		{
			if (Type == rhs.Type)
			{
				if (FilePath == rhs.FilePath)
					return Handle < rhs.Handle;
				return FilePath < rhs.FilePath;
			}
			return Type < rhs.Type;
		}
	};

	void EditorAssetManager::WriteImportedAssetsToDisc()
	{
		if (m_Project.Expired())
			return;

		auto project = m_Project.GetRef();
		const auto filepath = project->GetDirectory() / "ImportedAssets.yaml";

		std::set<ImportedAssetsEntry> sortedAssets;
		std::set<ImportedAssetsEntry> editorAssets;

		for (const auto& [handle, metadata] : m_Registry)
		{
			if (!metadata.IsValid() || metadata.IsMemoryAsset || !HasExistingFilePath(metadata))
				continue;

			if (metadata.IsEditorAsset)
			{
				editorAssets.emplace(metadata);
				continue;
			}

			sortedAssets.emplace(metadata);
		}

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "ImportedAssets" << YAML::Value << YAML::Node(sortedAssets);
		out << YAML::Key << "EditorAssets" << YAML::Value << YAML::Node(editorAssets);
		out << YAML::EndMap;

		if (!FileSystem::WriteString(filepath, out.c_str()))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to write imported assets to disc!");
			return;
		}
	}

	void EditorAssetManager::ReadImportedAssetsFromDisc()
	{
		SK_CORE_VERIFY(!m_Project.Expired());

		auto project = m_Project.GetRef();
		const auto filepath = m_Project.GetRef()->GetDirectory() / "ImportedAssets.yaml";

		if (!FileSystem::Exists(filepath))
		{
			SK_CORE_ERROR_TAG("Serialization", "ImportedAssets.yaml not found");
			return;
		}

		std::string filedata = FileSystem::ReadString(filepath);
		YAML::Node fileNode = YAML::Load(filedata);

		m_Registry.Clear();

		auto assetsNode = fileNode["ImportedAssets"];
		if (!assetsNode)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to find ImportedAssets root node in ImportedAssets.yaml");
			return;
		}

		for (auto entryNode : assetsNode)
		{
			auto entry = entryNode.as<ImportedAssetsEntry>();

			AssetMetaData metadata;
			metadata.Handle = entry.Handle;
			metadata.Type = entry.Type;
			metadata.FilePath = entry.FilePath;

			if (!HasExistingFilePath(metadata))
				continue;

			SK_CORE_VERIFY(m_Registry.Contains(metadata.Handle) == false);
			m_Registry.Add(metadata);
		}

		auto editorAssetsNode = fileNode["EditorAssets"];
		if (!editorAssetsNode)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to find EditorAssets root node im ImportedAssets.yaml");
			return;
		}

		for (auto entryNode : editorAssetsNode)
		{
			auto entry = entryNode.as<ImportedAssetsEntry>();
			AssetMetaData metadata;
			metadata.Handle = entry.Handle;
			metadata.Type = entry.Type;
			metadata.FilePath = entry.FilePath;
			metadata.IsEditorAsset = true;

			if (!FileSystem::Exists(metadata.FilePath))
				continue;

			SK_CORE_VERIFY(m_Registry.Contains(metadata.Handle) == false);
			m_Registry.Add(metadata);
			m_EditorAssets[utils::GetFilesystemKey(metadata.FilePath)] = metadata.Handle;
		}

	}

	const std::filesystem::path& EditorAssetManager::GetAssetsDirectoryFromProject() const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return m_Project.GetRef()->GetAssetsDirectory();
	}

}

namespace YAML {

	template<>
	struct convert<Shark::ImportedAssetsEntry>
	{
		static Node encode(const Shark::ImportedAssetsEntry& entry)
		{
			Node node(NodeType::Map);
			node.force_insert("Handle", entry.Handle);
			node.force_insert("Type", entry.Type);
			node.force_insert("FilePath", entry.FilePath);
			return node;
		}

		static bool decode(const Node& node, Shark::ImportedAssetsEntry& outEntry)
		{
			if (!node.IsMap() || node.size() != 3)
				return false;

			outEntry.Handle = node["Handle"].as<Shark::AssetHandle>();
			outEntry.Type = node["Type"].as<Shark::AssetType>();
			outEntry.FilePath = node["FilePath"].as<std::filesystem::path>();
			return true;
		}
	};

}
