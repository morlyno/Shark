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
		static const AssetMetaData& GetMetaData(const std::filesystem::path& filePath) { return GetMetaData(GetAssetHandleFromFilePath(MakeRelativePath(filePath))); }

		static bool IsValidAssetHandle(AssetHandle handle);
		static bool IsDataLoaded(AssetHandle handle);
		static bool IsMemoryAsset(AssetHandle handle);
		static bool IsImportedAsset(AssetHandle handle);

		static std::filesystem::path MakeRelativePath(const std::filesystem::path& filePath);
		static std::string MakeRelativePathString(const std::filesystem::path& filePath) { return MakeRelativePath(filePath).string(); }
		static std::filesystem::path GetFileSystemPath(const AssetMetaData& metadata);

		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filePath);

		static bool LoadAsset(AssetHandle handle);
		static bool SaveAsset(AssetHandle handle);
		static bool SaveAsset(Ref<Asset> asset);
		static void ReloadAsset(AssetHandle handle);

		static void UnloadAsset(AssetHandle handle);
		static void DeleteAsset(AssetHandle handle);

		static bool AddMemoryAssetToRegistry(AssetHandle handle, const std::string& directoryPath, const std::string& fileName);
		static AssetHandle ImportAsset(const std::filesystem::path& filePath);

		template<typename T = Asset>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			SK_PROFILE_FUNCTION();

			static_assert(std::is_base_of_v<Asset, T>, "GetAsset only works for types with base class Asset");
			if (!handle.IsValid())
				return nullptr;

			if (IsMemoryAsset(handle))
			{
				Ref<Asset> asset = s_MemoryAssets[handle];
				if (asset->GetAssetType() == T::GetStaticType())
					return asset.As<T>();
				return nullptr;
			}

			AssetMetaData& metadata = GetMetaDataInternal(handle);
			if (!metadata.IsValid())
				return nullptr;

			if (!metadata.IsDataLoaded)
			{
				Ref<Asset> asset = nullptr;
				metadata.IsDataLoaded = AssetSerializer::TryLoadData(asset, metadata);
				if (!metadata.IsDataLoaded)
					return nullptr;

				asset->Handle = handle;
				s_LoadedAssets[handle] = asset;

				if (asset->GetAssetType() == T::GetStaticType())
					return asset.As<T>();
				return nullptr;
			}

			return s_LoadedAssets[handle].As<T>();
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
			SK_CORE_ASSERT(GetAssetTypeFormFile(fileName) == T::GetStaticType());

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

			s_ImportedAssets[metadata.Handle] = metadata;

			Ref<T> asset = T::Create(std::forward<Args>(args)...);

			asset->Handle = metadata.Handle;
			s_LoadedAssets[metadata.Handle] = asset;

			AssetSerializer::Serialize(asset, metadata);
			WriteImportedAssetsToDisc();

			return asset;
		}

		template<typename T, typename... Args>
		static Ref<T> CreateMemoryAsset(Args&&... args)
		{
			SK_PROFILE_FUNCTION();

			static_assert(std::is_base_of_v<Asset, T>, "CreateMemoryAsset only works for types with base class Asset!");

			AssetHandle handle = AssetHandle::Generate();
			Ref<T> asset = T::Create(std::forward<Args>(args)...);
			asset->Handle = handle;
			s_MemoryAssets[handle] = asset;
			return asset;
		}

		static const auto& GetAssetRegistry() { return s_ImportedAssets; }
		static const auto& GetLoadedAssets() { return s_LoadedAssets; }
		static const auto& GetMemoryAssets() { return s_MemoryAssets; }

		// File Events
		static void OnFileEvents(const std::vector<FileChangedData>& fileEvents);
		static void OnAssetRenamed(const std::filesystem::path& oldFilePath, const std::filesystem::path& newFilePath);
		static void OnAssetDeleted(const std::filesystem::path& filePath);

	private:
		static AssetMetaData& GetMetaDataInternal(AssetHandle handle);
		static AssetType GetAssetTypeFormFilePath(const std::filesystem::path& filePath) { return GetAssetTypeFormFileExtention(filePath.extension().string()); }
		static AssetType GetAssetTypeFormFile(const std::string& file) { return GetAssetTypeFormFilePath(file); }
		static AssetType GetAssetTypeFormFileExtention(const std::string& fileExtention);

		static void WriteImportedAssetsToDisc();
		static void ReadImportedAssetsFromDisc();

	private:
		static std::unordered_map<AssetHandle, AssetMetaData> s_ImportedAssets;
		static std::unordered_map<AssetHandle, Ref<Asset>> s_LoadedAssets;
		static std::unordered_map<AssetHandle, Ref<Asset>> s_MemoryAssets;
	};

}
