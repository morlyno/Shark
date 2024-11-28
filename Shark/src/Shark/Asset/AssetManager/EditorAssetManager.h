#pragma once

#include "Shark/Asset/AssetMetadata.h"
#include "Shark/Asset/AssetManager/AssetRegistry.h"
#include "Shark/Asset/AssetManager/AssetManagerBase.h"

namespace Shark {

	class Project;
	class EditorAssetThread;

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

		AssetHandle AddAsset(Ref<Asset> asset, const std::filesystem::path& filepath);
		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) override;
		virtual bool ReloadAsset(AssetHandle handle) override;
		virtual void ReloadAssetAsync(AssetHandle handle) override;
		virtual bool DependenciesLoaded(AssetHandle handle, bool loadifNotReady) override;
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

		bool AssetMoved(AssetHandle handle, const std::filesystem::path& newpath);
		bool AssetRenamed(AssetHandle handle, const std::string& newName);
		bool AssetDeleted(AssetHandle handle);

	private:
		void WriteImportedAssetsToDisc();
		void ReadImportedAssetsFromDisc();

	private:
		std::filesystem::path GetAssetsDirectoryFromProject() const;

	public:
		Weak<Project> m_Project;
		Ref<EditorAssetThread> m_AssetThread;

		AssetRegistry m_Registry;
		LoadedAssetsMap m_LoadedAssets;
		LoadedAssetsMap m_MemoryOnlyAssets;

		std::unordered_map<std::filesystem::path, AssetHandle> m_EditorAssets;
	};

	template<typename TAsset, typename... TArgs>
	Ref<TAsset> EditorAssetManager::CreateAsset(const std::filesystem::path& filepath, TArgs&&... args)
	{
		static_assert(!std::is_same_v<Asset, TAsset>);
		static_assert(std::is_base_of_v<Asset, TAsset>, "CreateAsset only works for types with base class Asset!");

		Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
		AddAsset(asset, filepath);
		return asset;
	}
	
	template<typename TAsset, typename... TArgs>
	Ref<TAsset> EditorAssetManager::CreateRendererAsset(const std::filesystem::path& filepath, TArgs&&... args)
	{
		static_assert(!std::is_same_v<Asset, TAsset>);
		static_assert(std::is_base_of_v<Asset, TAsset>, "CreateAsset only works for types with base class Asset!");

		Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
		AddAsset(asset, filepath);
		return asset;
	}

}
