#include "skpch.h"
#include "ResourceManager.h"

#include "Shark/Utility/Utility.h"
#include "Shark/Utility/String.h"

#include "Shark/Utility/YAMLUtils.h"
#include <yaml-cpp/yaml.h>

namespace Shark {

	AssetRegistry ResourceManager::s_AssetRegistry;
	std::unordered_map<AssetHandle, Ref<Asset>> ResourceManager::s_LoadedAssets;
	std::unordered_map<AssetHandle, Ref<Asset>> ResourceManager::s_MemoryAssets;

	void ResourceManager::Init()
	{
		LoadAssetRegistry();
	}

	void ResourceManager::Shutdown()
	{
		SaveAssetRegistry();
		s_AssetRegistry.Clear();
		Unload();
	}

	void ResourceManager::Unload()
	{
		s_MemoryAssets.clear();
		s_LoadedAssets.clear();
	}

	const AssetMetaData& ResourceManager::GetMetaData(AssetHandle handle)
	{
		return GetMetaDataInternal(handle);
	}

	bool ResourceManager::IsValidAssetHandle(AssetHandle handle)
	{
		return handle.IsValid() && (s_AssetRegistry.Contains(handle) || IsMemoryAsset(handle));
	}

	bool ResourceManager::IsDataLoaded(AssetHandle handle)
	{
		return GetMetaData(handle).IsDataLoaded;
	}

	bool ResourceManager::IsMemoryAsset(AssetHandle handle)
	{
		return Utility::Contains(s_MemoryAssets, handle);
	}

	std::filesystem::path ResourceManager::GetRelativePath(const std::filesystem::path& filePath)
	{
		std::string path = filePath.string();
		if (path.find(Project::GetAssetsPath().string()) != std::string::npos)
			return FileSystem::FormatDefaultCopy(std::filesystem::relative(filePath, Project::GetAssetsPath()));
		return FileSystem::FormatDefaultCopy(std::filesystem::relative(Project::GetAssetsPath() / filePath, Project::GetAssetsPath()));
	}

	AssetHandle ResourceManager::GetAssetHandleFromFilePath(const std::filesystem::path& filePath)
	{
		auto relativePath = GetRelativePath(filePath);
		for (auto [handle, metadata] : s_AssetRegistry)
			if (metadata.FilePath == relativePath)
				return metadata.Handle;
		return AssetHandle();
	}

	bool ResourceManager::LoadAsset(AssetHandle handle)
	{
		auto& metadata = GetMetaDataInternal(handle);
		if (!metadata.IsDataLoaded)
		{
			Ref<Asset> asset = nullptr;
			metadata.IsDataLoaded = AssetSerializer::TryLoadData(asset, metadata);
			asset->Handle = handle;
			s_LoadedAssets[handle] = asset;
		}
		return metadata.IsDataLoaded;
	}

	bool ResourceManager::SaveAsset(AssetHandle handle)
	{
		if (IsMemoryAsset(handle))
			return false;

		const auto& metadata = GetMetaDataInternal(handle);
		if (!metadata.IsDataLoaded)
			return false;

		return AssetSerializer::Serialize(GetAsset(handle), metadata);
	}

	void ResourceManager::ReloadAsset(AssetHandle handle)
	{
		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (metadata.IsDataLoaded)
		{
			UnloadAsset(handle);
			LoadAsset(handle);
		}
	}

	bool ResourceManager::AddMemoryAssetToRegistry(AssetHandle handle, const std::string& directoryPath, const std::string& fileName)
	{
		if (!IsMemoryAsset(handle))
			return false;

		Ref<Asset> asset = s_MemoryAssets.at(handle);

		AssetMetaData metadata;
		metadata.Handle = asset->Handle;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = GetRelativePath(directoryPath + "/" + fileName);
		metadata.IsDataLoaded = true;

		if (FileSystem::Exists(metadata.FilePath))
		{
			uint32_t count = 1;
			bool foundValidFilePath = false;
			while (!foundValidFilePath)
			{
				metadata.FilePath = fmt::format("{}/{} ({:2})", directoryPath, fileName, count++);
				foundValidFilePath = !FileSystem::Exists(metadata.FilePath);
			}
		}

		if (!AssetSerializer::Serialize(asset, metadata))
			return false;

		s_AssetRegistry[metadata.Handle] = metadata;
		SaveAssetRegistry();

		s_LoadedAssets[handle] = asset;
		s_MemoryAssets.erase(handle);

		return true;
	}

	void ResourceManager::UnloadAsset(AssetHandle handle)
	{
		SK_CORE_ASSERT(IsValidAssetHandle(handle));
		if (IsMemoryAsset(handle))
			s_MemoryAssets.erase(handle);
		else
			s_LoadedAssets.erase(handle);
		AssetMetaData& metadata = GetMetaDataInternal(handle);
		metadata.IsDataLoaded = false;
	}

	void ResourceManager::DeleteAsset(AssetHandle handle)
	{
		if (IsMemoryAsset(handle))
		{
			s_MemoryAssets.erase(handle);
			return;
		}

		s_LoadedAssets.erase(handle);
		s_AssetRegistry.Remove(handle);
		SaveAssetRegistry();

	}

	AssetHandle ResourceManager::ImportAsset(const std::filesystem::path& filePath)
	{
		AssetType type = GetAssetTypeFormFilePath(filePath);
		if (type == AssetType::None)
			return 0;

		if (AssetHandle handle = GetAssetHandleFromFilePath(filePath))
			return handle;

		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.FilePath = GetRelativePath(filePath);
		metadata.Type = type;
		metadata.IsDataLoaded = false;
		s_AssetRegistry[metadata.Handle] = metadata;
		return metadata.Handle;
	}

	static AssetMetaData s_NullMetaData;
	AssetMetaData& ResourceManager::GetMetaDataInternal(AssetHandle handle)
	{
		if (s_AssetRegistry.Contains(handle))
			return s_AssetRegistry.Get(handle);
		return s_NullMetaData;
	}

	AssetType ResourceManager::GetAssetTypeFormFileExtention(const std::string& fileExtention)
	{
		std::string extention = String::ToLowerCopy(fileExtention);
		if (!Utility::Contains(AssetExtentionMap, extention))
			return AssetType::None;
		return AssetExtentionMap[extention];
	}

	void ResourceManager::SaveAssetRegistry()
	{
		const auto filePath = Project::GetProjectDirectory() / "AssetRegistry.skar";

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::Value;
		out << YAML::BeginSeq;

		for (auto [handle, metadata] : s_AssetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.Type);
			out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filePath);
		if (!fout)
		{
			SK_CORE_ERROR("Output File Stream Failed");
			return;
		}

		fout << out.c_str();
		fout.close();
	}

	void ResourceManager::LoadAssetRegistry()
	{
		const auto filePath = Project::GetProjectDirectory() / "AssetRegistry.skar";

		if (!FileSystem::Exists(filePath))
		{
			SK_CORE_ERROR("Asset Registry File dosn't exist");
			return;
		}

		YAML::Node in = YAML::LoadFile(filePath);

		if (!in["Assets"])
		{
			SK_CORE_ERROR("Invalid AssetRegistry file");
			return;
		}

		auto assets = in["Assets"];
		for (auto asset : assets)
		{
			auto handle = asset["Handle"];
			auto type = asset["Type"];
			auto filePath = asset["FilePath"];

			SK_CORE_ASSERT(handle);
			SK_CORE_ASSERT(type);
			SK_CORE_ASSERT(filePath);

			AssetMetaData metadata;
			metadata.Handle = handle.as<UUID>();
			metadata.Type = StringToAssetType(type.as<std::string>());
			metadata.FilePath = filePath.as<std::filesystem::path>();
			metadata.IsDataLoaded = false;

			s_AssetRegistry[metadata.Handle] = metadata;
		}
	}

}