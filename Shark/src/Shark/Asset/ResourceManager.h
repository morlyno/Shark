#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetTypes.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	class ResourceManager
	{
	public:
		static void Init();
		static void Shutdown();
		static void Unload();

		static const AssetMetaData& GetMetaData(AssetHandle handle);
		static const AssetMetaData& GetMetaData(Ref<Asset> asset) { return GetMetaData(asset->Handle); }
		static const AssetMetaData& GetMetaData(const std::filesystem::path& filePath) { return GetMetaData(GetAssetHandleFromFilePath(filePath)); }

		static AssetType GetAssetTypeFormFilePath(const std::filesystem::path& filePath);
		static AssetType GetAssetTypeFormExtension(const std::string& fileExtension);

		static bool IsValidAssetHandle(AssetHandle handle);
		static bool IsMemoryAsset(AssetHandle handle);
		static bool IsFileImported(const std::filesystem::path& path);

		static bool HasExistingFilePath(const AssetMetaData& metadata);
		static bool HasExistingFilePath(Ref<Asset> asset) { return HasExistingFilePath(GetMetaData(asset->Handle)); }

		static std::filesystem::path MakeRelativePath(const std::filesystem::path& filePath);
		static std::string MakeRelativePathString(const std::filesystem::path& filePath) { return MakeRelativePath(filePath).string(); }
		static std::filesystem::path GetFileSystemPath(const AssetMetaData& metadata);
		static std::filesystem::path GetProjectPath(const AssetMetaData& metadata);
		static std::string GetFileSystemPathString(const AssetMetaData& metadata) { return GetFileSystemPath(metadata).string(); }

		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filePath);

		static bool LoadAsset(AssetHandle handle);
		static bool SaveAsset(AssetHandle handle);
		static void ReloadAsset(AssetHandle handle);

		static void UnloadAsset(AssetHandle handle);
		static void DeleteAsset(AssetHandle handle);

		static bool ImportMemoryAsset(AssetHandle handle, const std::string& directoryPath, const std::string& fileName);
		static AssetHandle ImportAsset(const std::filesystem::path& filePath);

		template<typename T = Asset>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, T>, "GetAsset only works for types with base class Asset");

			AssetMetaData& metadata = GetMetaDataInternal(handle);
			if (!metadata.IsValid())
				return nullptr;

			if (metadata.IsMemoryAsset)
			{
				Ref<Asset> asset = s_Data->LoadedAssets.at(handle);
				if (asset->GetAssetType() == T::GetStaticType())
					return asset.As<T>();
				return nullptr;
			}

			if (!metadata.IsDataLoaded)
			{
				Ref<Asset> asset = nullptr;
				metadata.IsDataLoaded = AssetSerializer::TryLoadData(asset, metadata);
				if (!metadata.IsDataLoaded)
					return asset.As<T>(); // returns asset with Error Flags set

				s_Data->LoadedAssets[handle] = asset;

				if (asset->GetAssetType() == T::GetStaticType())
					return asset.As<T>();
				return nullptr;
			}

			return s_Data->LoadedAssets.at(handle).As<T>();
		}

		template<typename T = Asset>
		static Ref<T> GetLoadedAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, T>, "GetAsset only works for types with base class Asset");

			const AssetMetaData& metadata = GetMetaDataInternal(handle);
			if (!metadata.IsValid())
				return nullptr;

			if (metadata.IsDataLoaded)
				return s_Data->LoadedAssets.at(handle).As<T>();
			return nullptr;
		}

		// Directory + FileName:
		// Scenes/SampleScene.skscene
		// Directory: Scenes
		// FileName: SampleScene.skscene

		template<typename T, typename... Args>
		static Ref<T> CreateAsset(const std::string& directoryPath, const std::string& fileName, Args&&... args)
		{
			SK_PROFILE_FUNCTION();

			static_assert(!std::is_same_v<Asset, T>);
			static_assert(std::is_base_of_v<Asset, T>, "CreateAsset only works for types with base class Asset!");
			//SK_CORE_ASSERT(GetAssetTypeFormFileName(fileName) == T::GetStaticType());

			std::string dirPath = MakeRelativePathString(directoryPath);

			AssetMetaData metadata;
			metadata.Handle = AssetHandle::Generate();
			metadata.Type = T::GetStaticType();
			metadata.FilePath = dirPath.size() > 1 ? dirPath + "/" + fileName : fileName;
			metadata.IsDataLoaded = true;

			auto filesystemPath = GetFileSystemPath(metadata);
			if (std::filesystem::exists(filesystemPath))
			{
				uint32_t count = 1;
				bool foundValidFilePath = false;

				std::filesystem::path pathFileName = fileName;
				std::string path = (directoryPath / pathFileName.stem()).generic_string();
				std::string extention = pathFileName.extension().string();

				while (!foundValidFilePath)
				{
					filesystemPath = fmt::format("{} ({:2}){}", path, count++, extention);
					foundValidFilePath = !std::filesystem::exists(filesystemPath);
				}
				metadata.FilePath = MakeRelativePath(filesystemPath);
			}

			s_Data->ImportedAssets[metadata.Handle] = metadata;

			Ref<T> asset = T::Create(std::forward<Args>(args)...);

			asset->Handle = metadata.Handle;
			s_Data->LoadedAssets[metadata.Handle] = asset;

			AssetSerializer::Serialize(asset, metadata);
			WriteImportedAssetsToDisc();

			return asset;
		}

		template<typename TAsset, typename... TArgs>
		static Ref<TAsset> CreateMemoryAsset(TArgs&&... args)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "CreateMemoryAsset only works for types with base class Asset!");

			AssetMetaData metadata;
			metadata.Type = TAsset::GetStaticType();
			metadata.Handle = AssetHandle::Generate();
			metadata.IsMemoryAsset = true;
			s_Data->ImportedAssets[metadata.Handle] = metadata;

			Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
			asset->Handle = metadata.Handle;
			s_Data->LoadedAssets[metadata.Handle] = asset;

			return asset;
		}

		static const auto& GetAssetRegistry() { return s_Data->ImportedAssets; }
		static const auto& GetLoadedAssets() { return s_Data->LoadedAssets; }

		// File Events
		static void OnFileEvents(const std::vector<FileChangedData>& fileEvents);
		static void OnAssetRenamed(const std::filesystem::path& oldFilePath, const std::filesystem::path& newFilePath);
		static void OnAssetDeleted(const std::filesystem::path& filePath);

	private:
		static AssetMetaData& GetMetaDataInternal(AssetHandle handle);
		static void WriteImportedAssetsToDisc();
		static void ReadImportedAssetsFromDisc();

		struct ResourceManagerData
		{
			std::unordered_map<AssetHandle, AssetMetaData> ImportedAssets;
			std::unordered_map<AssetHandle, Ref<Asset>> LoadedAssets;
		};
		static ResourceManagerData* s_Data;
	};

}
