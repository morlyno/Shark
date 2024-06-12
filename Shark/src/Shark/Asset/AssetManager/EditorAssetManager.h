#pragma once

#include "Shark/Asset/AssetMetadata.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"
#include "Shark/Asset/AssetManager/AssetManagerBase.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Asset/AssetThread/EditorAssetThread.h"
#include "Shark/File/FileSystem.h"

namespace Shark {

	class Project;

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager(Ref<Project> project);
		~EditorAssetManager();

		void SerializeImportedAssets();

		virtual AssetType GetAssetType(AssetHandle handle) override;
		virtual Ref<Asset> GetAsset(AssetHandle handle) override;
		virtual AsyncLoadResult<Asset> GetAssetAsync(AssetHandle handle) override;
		virtual Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle) override;

		virtual std::vector<AssetHandle> GetAllAssetsOfType(AssetType assetType) override;

		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) override;
		virtual bool ReloadAsset(AssetHandle handle) override;
		virtual void ReloadAssetAsync(AssetHandle handle) override;
		virtual bool IsFullyLoaded(AssetHandle handle, bool loadifNotReady) override;
		virtual bool IsValidAssetHandle(AssetHandle handle) override;
		virtual bool IsMemoryAsset(AssetHandle handle) override;
		virtual bool IsAssetLoaded(AssetHandle handle) override;
		virtual void DeleteAsset(AssetHandle handle) override;
		virtual void DeleteMemoryAsset(AssetHandle handle) override;
		bool SaveAsset(AssetHandle handle);

		virtual void WaitUntilIdle() override;
		virtual void SyncWithAssetThread() override;

		Ref<Asset> GetPlaceholder(AssetType assetType);

		template<typename TAsset, typename... TArgs>
		Ref<TAsset> CreateAsset(const std::filesystem::path& filepath, TArgs&&... args);
		template<typename TAsset, typename... TArgs>
		Ref<TAsset> CreateRendererAsset(const std::filesystem::path& filepath, TArgs&&... args);

		AssetHandle GetEditorAsset(const std::filesystem::path& filepath);
		AssetHandle AddEditorAsset(const std::filesystem::path& filepath) { return AddEditorAsset(AssetHandle::Generate(), filepath); }
		AssetHandle AddEditorAsset(AssetHandle handle, const std::filesystem::path& filepath);
		AssetHandle AddEditorAsset(Ref<Asset> asset, const std::filesystem::path& filepath);
		bool HasEditorAsset(const std::filesystem::path& filepath) const;

		const AssetMetaData& GetMetadata(AssetHandle handle) const;
		const AssetMetaData& GetMetadata(Ref<Asset> asset) const;
		const AssetMetaData& GetMetadata(const std::filesystem::path& filepath) const;
		AssetHandle GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const;
		bool IsFileImported(const std::filesystem::path& filepath) const;

		std::filesystem::path MakeRelativePath(const std::filesystem::path& filepath) const;
		std::string MakeRelativePathString(const std::filesystem::path& filepath) const;

		std::filesystem::path GetFilesystemPath(AssetHandle handle) const;
		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata) const;
		std::filesystem::path GetProjectPath(const AssetMetaData& metadata) const;

		bool HasExistingFilePath(const AssetMetaData& metadata);
		bool HasExistingFilePath(AssetHandle handle);

		bool ImportMemoryAsset(AssetHandle handle, const std::string& directory, const std::string& filename);
		AssetHandle ImportAsset(const std::filesystem::path& filepath);

		const LoadedAssetsMap& GetLoadedAssets() const;
		AssetRegistry& GetAssetRegistry();
		const AssetRegistry& GetAssetRegistry() const;

		bool AssetMoved(AssetHandle asset, const std::filesystem::path& newpath);
		bool AssetRenamed(AssetHandle asset, const std::string& newName);

	private:
		AssetMetaData& GetMetadataInternal(AssetHandle handle) { return m_Registry.TryGet(handle); }
		const AssetMetaData& GetMetadataInternal(AssetHandle handle) const { return m_Registry.TryGet(handle); }

		void WriteImportedAssetsToDisc();
		void ReadImportedAssetsFromDisc();

	private:
		const std::filesystem::path GetAssetsDirectoryFromProject() const;

	public:
		Weak<Project> m_Project;
		Ref<EditorAssetThread> m_AssetThread;

		AssetRegistry m_Registry;
		LoadedAssetsMap m_LoadedAssets;

		std::unordered_map<std::filesystem::path, AssetHandle> m_EditorAssets;
	};

	template<typename TAsset, typename... TArgs>
	Ref<TAsset> EditorAssetManager::CreateAsset(const std::filesystem::path& filepath, TArgs&&... args)
	{
		static_assert(!std::is_same_v<Asset, TAsset>);
		static_assert(std::is_base_of_v<Asset, TAsset>, "CreateAsset only works for types with base class Asset!");

		std::string assetsPath = MakeRelativePathString(filepath);

		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.Type = TAsset::GetStaticType();
		metadata.FilePath = assetsPath;
		metadata.Status = AssetStatus::Ready;

		// Make sure metadata.FilePath is unique
		if (HasExistingFilePath(metadata))
		{
			uint32_t count = 1;
			bool validFilepath = false;
			std::filesystem::path fsPath = GetFilesystemPath(metadata);

			while (!validFilepath)
			{
				FileSystem::ReplaceStem(fsPath, fmt::format("{} ({:2})", FileSystem::GetStemString(fsPath), count));
				validFilepath = !FileSystem::Exists(fsPath);
			}
			metadata.FilePath = MakeRelativePath(fsPath);
		}

		Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
		asset->Handle = metadata.Handle;

		m_Registry[metadata.Handle] = metadata;
		m_LoadedAssets[metadata.Handle] = asset;
		AssetSerializer::Serialize(asset, metadata);
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("AssetManager", "Asset Created (Type: {0}, Handle: 0x{1:x}, FilePath: {2}", ToString(metadata.Type), metadata.Handle, metadata.FilePath);
		return asset;
	}
	
	template<typename TAsset, typename... TArgs>
	Ref<TAsset> EditorAssetManager::CreateRendererAsset(const std::filesystem::path& filepath, TArgs&&... args)
	{
		static_assert(!std::is_same_v<Asset, TAsset>);
		static_assert(std::is_base_of_v<Asset, TAsset>, "CreateAsset only works for types with base class Asset!");

		std::string assetsPath = MakeRelativePathString(filepath);

		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.Type = TAsset::GetStaticType();
		metadata.FilePath = assetsPath;
		metadata.Status = AssetStatus::Ready;

		// Make sure metadata.FilePath is unique
		if (HasExistingFilePath(metadata))
		{
			uint32_t count = 1;
			bool validFilepath = false;
			std::filesystem::path fsPath = GetFilesystemPath(metadata);

			while (!validFilepath)
			{
				FileSystem::ReplaceStem(fsPath, fmt::format("{} ({:2})", FileSystem::GetStemString(fsPath), count));
				validFilepath = !FileSystem::Exists(fsPath);
			}
			metadata.FilePath = MakeRelativePath(fsPath);
		}

		Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
		asset->Handle = metadata.Handle;

		m_Registry[metadata.Handle] = metadata;
		m_LoadedAssets[metadata.Handle] = asset;
		AssetSerializer::Serialize(asset, metadata);
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("AssetManager", "Asset Created (Type: {0}, Handle: 0x{1:x}, FilePath: {2}", ToString(metadata.Type), metadata.Handle, metadata.FilePath);
		return asset;
	}

}
