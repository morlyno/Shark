#pragma once

#include "Shark/Asset/AssetManagerBase.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"

namespace Shark {

	class Project;
	using AssetMetadataMap = std::unordered_map<AssetHandle, AssetMetaData>;

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager(Ref<Project> project);
		~EditorAssetManager();

		void SerializeImportedAssets();

		virtual Ref<Asset> GetAsset(AssetHandle handle) override;
		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) override;
		template<typename TAsset, typename... TArgs>
		Ref<TAsset> CreateAsset(const std::filesystem::path& filepath, TArgs&&... args);

		AssetHandle GetEditorAsset(const std::filesystem::path& filepath);
		AssetHandle AddEditorAsset(const std::filesystem::path& filepath) { return AddEditorAsset(AssetHandle::Generate(), filepath); }
		AssetHandle AddEditorAsset(AssetHandle handle, const std::filesystem::path& filepath);
		AssetHandle AddEditorAsset(Ref<Asset> asset, const std::filesystem::path& filepath);
		bool HasEditorAsset(const std::filesystem::path& filepath) const;

		virtual AssetType GetAssetType(AssetHandle handle) const override;
		virtual bool IsMemoryAsset(AssetHandle handle) const override;
		virtual bool IsValidAssetHandle(AssetHandle handle) const override;
		virtual bool IsAssetLoaded(AssetHandle handle) const override;
		bool IsFileImported(const std::filesystem::path& filepath) const;

		const AssetMetaData& GetMetadata(AssetHandle handle) const;
		const AssetMetaData& GetMetadata(Ref<Asset> asset) const;
		const AssetMetaData& GetMetadata(const std::filesystem::path& filepath) const;
		AssetHandle GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const;

		std::filesystem::path MakeRelativePath(const std::filesystem::path& filepath) const;
		std::string MakeRelativePathString(const std::filesystem::path& filepath) const;

		std::filesystem::path GetFilesystemPath(AssetHandle handle) const;
		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata) const;
		std::filesystem::path GetProjectPath(const AssetMetaData& metadata) const;

		bool HasExistingFilePath(const AssetMetaData& metadata);
		bool HasExistingFilePath(AssetHandle handle);

		bool LoadAsset(AssetHandle handle);
		bool SaveAsset(AssetHandle handle);
		bool ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);
		void RemoveAsset(AssetHandle handle);

		bool ImportMemoryAsset(AssetHandle handle, const std::string& directory, const std::string& filename);
		AssetHandle ImportAsset(const std::filesystem::path& filepath);
		AssetHandle ImportAssetFrom(const std::filesystem::path& sourceFile, const std::filesystem::path& destinationDirectory, const std::string& overrideFilename = {});

		const AssetsMap& GetLoadedAssets() const;
		const AssetMetadataMap& GetAssetMetadataMap() const;

		// File Events
		void OnAssetCreated(const std::filesystem::path& filepath);
		void OnAssetDeleted(const std::filesystem::path& filepath);
		void OnAssetRenamed(const std::filesystem::path& oldFilepath, const std::string& newName);

	private:
		AssetMetaData& GetMetadataInternal(AssetHandle handle);
		const AssetMetaData& GetMetadataInternal(AssetHandle handle) const;

		void WriteImportedAssetsToDisc();
		void ReadImportedAssetsFromDisc();

	private:
		const std::filesystem::path GetAssetsDirectoryFromProject() const;

	public:
		Weak<Project> m_Project;
		AssetsMap m_LoadedAssets;
		AssetMetadataMap m_ImportedAssets;

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
		metadata.IsDataLoaded = true;

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

		m_ImportedAssets[metadata.Handle] = metadata;
		m_LoadedAssets[metadata.Handle] = asset;
		AssetSerializer::Serialize(asset, metadata);
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("AssetManager", "Asset Created (Type: {0}, Handle: 0x{1:x}, FilePath: {2}", ToString(metadata.Type), metadata.Handle, metadata.FilePath);
		return asset;
	}

}
