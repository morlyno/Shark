#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetTypes.h"
#include "Shark/Asset/AssetRegistry.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class ResourceManager
	{
	public:
		static void Init();
		static void Shutdown();
		static void Unload();

		static const AssetMetaData& GetMetaData(AssetHandle handle);
		static const AssetMetaData& GetMetaData(Ref<Asset> asset) { return GetMetaData(asset->Handle); }
		static const AssetMetaData& GetMetaData(const std::filesystem::path& filePath) { return GetMetaData(GetAssetHandleFromFilePath(GetRelativePath(filePath))); }

		static bool IsValidAssetHandle(AssetHandle handle);
		static bool IsDataLoaded(AssetHandle handle);
		static bool IsMemoryAsset(AssetHandle handle);

		static std::filesystem::path GetRelativePath(const std::filesystem::path& filePath);
		static std::string GetRelativePathString(const std::filesystem::path& filePath) { return GetRelativePath(filePath).string(); }
		static std::filesystem::path GetFileSystemPath(const AssetMetaData& metadata) { return Project::GetAssetsPath() / metadata.FilePath; }

		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filePath);

		static bool LoadAsset(Ref<Asset> asset);
		static bool SaveAsset(AssetHandle handle);

		static bool AddMemoryAssetToRegistry(AssetHandle handle, const std::string& directoryPath, const std::string& fileName);

		static void UnloadAsset(AssetHandle handle);
		static void DeleteAsset(AssetHandle handle);

		static AssetHandle ImportAsset(const std::filesystem::path& filePath);

		template<typename T = Asset>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, T>, "GetAsset only works for types with base class Asset");
			if (!handle.IsValid())
				return nullptr;

			if (IsMemoryAsset(handle))
				return s_MemoryAssets[handle].As<T>();

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
				return asset.As<T>();
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
			static_assert(!std::is_same_v<Asset, T>);
			static_assert(std::is_base_of_v<Asset, T>, "CreateAsset only works for types with base class Asset!");
			SK_CORE_ASSERT(GetAssetTypeFormFile(fileName) == T::GetStaticType());

			AssetMetaData metadata;
			metadata.Handle = AssetHandle::Generate();
			metadata.Type = T::GetStaticType();
			metadata.FilePath = GetRelativePath(directoryPath + "/" + file);
			metadata.IsDataLoaded = true;

			auto filesystemPath = GetFileSystemPath(metadata);
			if (FileSystem::Exists(filesystemPath))
			{
				uint32_t count = 1;
				bool foundValidFilePath = false;
				while (!foundValidFilePath)
				{
					filesystemPath = fmt::format("{}/{} ({:2})", directoryPath, fileName, count++);
					foundValidFilePath = !FileSystem::Exists(filesystemPath);
				}
				metadata.FilePath = GetRelativePath(filesystemPath);
			}

			s_AssetRegistry[metadata.Handle] = metadata;
			SaveAssetRegistry();

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			s_LoadedAssets[metadata.Handle] = asset;

			AssetSerializer::Serialize(asset, metadata);

			return asset;
		}

		template<typename T, typename... Args>
		static Ref<T> CreateMemoryAsset(Args&&... args)
		{
			static_assert(std::is_base_of_v<Asset, T>, "CreateMemoryAsset only works for types with base class Asset!");

			AssetHandle handle = AssetHandle::Generate();
			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = handle;
			s_MemoryAssets[handle] = asset;
			return asset;
		}

		static const AssetRegistry& GetAssetRegistry() { return s_AssetRegistry; }
		static const auto& GetLoadedAssets() { return s_LoadedAssets; }
		static const auto& GetMemoryAssets() { return s_MemoryAssets; }

	private:
		static AssetMetaData& GetMetaDataInternal(AssetHandle handle);
		static AssetType GetAssetTypeFormFilePath(const std::filesystem::path& filePath) { return GetAssetTypeFormFileExtention(filePath.extension().string()); }
		static AssetType GetAssetTypeFormFile(const std::string& file) { return GetAssetTypeFormFilePath(file); }
		static AssetType GetAssetTypeFormFileExtention(const std::string& fileExtention);

		static void SaveAssetRegistry();
		static void LoadAssetRegistry();

	private:
		static AssetRegistry s_AssetRegistry;
		static std::unordered_map<AssetHandle, Ref<Asset>> s_LoadedAssets;
		static std::unordered_map<AssetHandle, Ref<Asset>> s_MemoryAssets;
	};

}
