#include "skpch.h"
#include "ResourceManager.h"

#include "Shark/Utils/String.h"

#include "Shark/Utils/YAMLUtils.h"
#include <yaml-cpp/yaml.h>

namespace Shark {

	namespace utils {

		static void SetFlag(AssetHandle handle, AssetFlag::Type flag, bool enabled)
		{
			if (auto asset = ResourceManager::GetLoadedAsset(handle))
				asset->SetFlag(flag, enabled);
		}

	}

	std::unordered_map<AssetHandle, AssetMetaData> ResourceManager::s_ImportedAssets;
	std::unordered_map<AssetHandle, Ref<Asset>> ResourceManager::s_LoadedAssets;
	std::unordered_map<AssetHandle, Ref<Asset>> ResourceManager::s_MemoryAssets;

	void ResourceManager::Init()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(s_ImportedAssets.empty());
		SK_CORE_ASSERT(s_MemoryAssets.empty());
		SK_CORE_ASSERT(s_LoadedAssets.empty());

		AssetSerializer::RegisterSerializers();
		ReadImportedAssetsFromDisc();
	}

	void ResourceManager::Shutdown()
	{
		SK_PROFILE_FUNCTION();

		WriteImportedAssetsToDisc();
		Unload();
		AssetSerializer::ReleaseSerializers();
	}

	void ResourceManager::Unload()
	{
		SK_PROFILE_FUNCTION();

		s_ImportedAssets.clear();
		s_MemoryAssets.clear();
		s_LoadedAssets.clear();
	}

	const AssetMetaData& ResourceManager::GetMetaData(AssetHandle handle)
	{
		return GetMetaDataInternal(handle);
	}

	bool ResourceManager::IsValidAssetHandle(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		return handle.IsValid() && (IsImportedAsset(handle) || IsMemoryAsset(handle));
	}

	bool ResourceManager::IsDataLoaded(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		return GetMetaData(handle).IsDataLoaded;
	}

	bool ResourceManager::IsMemoryAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		return s_MemoryAssets.find(handle) != s_MemoryAssets.end();
	}

	bool ResourceManager::IsImportedAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		return s_ImportedAssets.find(handle) != s_ImportedAssets.end();
	}

	bool ResourceManager::IsLoadedAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		return s_LoadedAssets.find(handle) != s_LoadedAssets.end();
	}

	bool ResourceManager::HasExistingFilePath(const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		return std::filesystem::exists(GetFileSystemPath(metadata));
	}

	std::filesystem::path ResourceManager::MakeRelativePath(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		auto relativePath = std::filesystem::relative(Project::AssetsPath() / filePath, Project::AssetsPath());
		String::FormatDefault(relativePath);
		return relativePath;
	}

	std::filesystem::path ResourceManager::GetFileSystemPath(const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		if (metadata.FilePath.empty())
			return {};

		std::filesystem::path fsPath = Project::AssetsPath() / metadata.FilePath;
		return fsPath.lexically_normal();
	}

	AssetHandle ResourceManager::GetAssetHandleFromFilePath(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		auto relativePath = MakeRelativePath(filePath);
		for (const auto& [handle, metadata] : s_ImportedAssets)
			if (metadata.FilePath == relativePath)
				return metadata.Handle;
		return AssetHandle();
	}

	bool ResourceManager::LoadAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

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
		SK_PROFILE_FUNCTION();

		if (IsMemoryAsset(handle))
			return false;

		const auto& metadata = GetMetaDataInternal(handle);
		if (!metadata.IsDataLoaded)
			return false;

		return AssetSerializer::Serialize(GetAsset(handle), metadata);
	}

	bool ResourceManager::SaveAsset(Ref<Asset> asset)
	{
		SK_PROFILE_FUNCTION();

		if (!asset)
			return false;

		if (IsMemoryAsset(asset->Handle))
			return false;

		return AssetSerializer::Serialize(asset, GetMetaDataInternal(asset->Handle));
	}

	void ResourceManager::ReloadAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (metadata.IsDataLoaded)
		{
			Ref<Asset> asset = s_LoadedAssets.at(handle);
			SK_CORE_ASSERT(asset);
			asset->SetFlag(AssetFlag::Unloaded, true);
			asset->Flags |= AssetFlag::Unloaded;
			UnloadAsset(handle);
		}
		LoadAsset(handle);
	}

	void ResourceManager::UnloadAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(IsValidAssetHandle(handle));
		if (IsMemoryAsset(handle))
		{
			utils::SetFlag(handle, AssetFlag::Unloaded, true);
			s_MemoryAssets.erase(handle);
			return;
		}

		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (metadata.IsDataLoaded)
			utils::SetFlag(handle, AssetFlag::Unloaded, true);

		metadata.IsDataLoaded = false;
		s_LoadedAssets.erase(handle);
	}

	void ResourceManager::DeleteAsset(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		if (IsMemoryAsset(handle))
		{
			s_MemoryAssets.erase(handle);
			return;
		}

		s_LoadedAssets.erase(handle);
		s_ImportedAssets.erase(handle);
		WriteImportedAssetsToDisc();
	}

	bool ResourceManager::ImportMemoryAsset(AssetHandle handle, const std::string& directoryPath, const std::string& fileName)

	{
		SK_PROFILE_FUNCTION();

		if (!IsMemoryAsset(handle))
			return false;

		Ref<Asset> asset = s_MemoryAssets.at(handle);

		AssetMetaData metadata;
		metadata.Handle = asset->Handle;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = MakeRelativePath(directoryPath + "/" + fileName);
		metadata.IsDataLoaded = true;

		if (std::filesystem::exists(metadata.FilePath))
		{
			uint32_t count = 1;
			bool foundValidFilePath = false;
			while (!foundValidFilePath)
			{
				metadata.FilePath = fmt::format("{}/{} ({:2})", directoryPath, fileName, count++);
				foundValidFilePath = !std::filesystem::exists(metadata.FilePath);
			}
		}

		if (!AssetSerializer::Serialize(asset, metadata))
			return false;

		s_ImportedAssets[metadata.Handle] = metadata;
		WriteImportedAssetsToDisc();

		s_LoadedAssets[handle] = asset;
		s_MemoryAssets.erase(handle);

		return true;
	}

	AssetHandle ResourceManager::ImportAsset(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		AssetType type = GetAssetTypeFormFilePath(filePath);
		if (type == AssetType::None)
			return 0;

		if (!std::filesystem::exists(Project::AssetsPath() / filePath))
			return 0;

		if (AssetHandle handle = GetAssetHandleFromFilePath(filePath))
			return handle;


		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.FilePath = MakeRelativePath(filePath);
		metadata.Type = type;
		metadata.IsDataLoaded = false;
		s_ImportedAssets[metadata.Handle] = metadata;
		WriteImportedAssetsToDisc();

		SK_CORE_INFO("Imported Asset");
		SK_CORE_TRACE(" => Handle: 0x{:x}, Type: {}, FilePath: {}", metadata.Handle, AssetTypeToString(metadata.Type), metadata.FilePath);
		return metadata.Handle;
	}

	void ResourceManager::OnFileEvents(const std::vector<FileChangedData>& fileEvents)
	{
		std::filesystem::path oldFilePath;
		for (const FileChangedData& event : fileEvents)
		{
			if (event.FileEvent == FileEvent::NewName && !oldFilePath.empty())
			{
				OnAssetRenamed(oldFilePath, event.FilePath);
				break;
			}

			AssetType assetType = GetAssetTypeFormFilePath(event.FilePath);
			if (assetType != AssetType::None)
			{
				switch (event.FileEvent)
				{
					case FileEvent::Deleted: OnAssetDeleted(event.FilePath); break;
					case FileEvent::OldName: oldFilePath = event.FilePath; break;
					case FileEvent::NewName: OnAssetRenamed(oldFilePath, event.FilePath);
				}
			}
		}
	}

	void ResourceManager::OnAssetRenamed(const std::filesystem::path& oldFilePath, const std::filesystem::path& newFilePath)
	{
		SK_CORE_ASSERT(oldFilePath.is_absolute());
		SK_CORE_ASSERT(newFilePath.is_absolute());
		
		SK_CORE_ASSERT(oldFilePath != newFilePath, "don't know if this can happen");
		if (oldFilePath == newFilePath)
			return;


		AssetHandle handle = GetAssetHandleFromFilePath(oldFilePath);
		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (!handle.IsValid())
			return;

		SK_CORE_ASSERT(std::filesystem::exists(newFilePath));
		auto relativePath = MakeRelativePath(newFilePath);
		metadata.FilePath = relativePath;
		WriteImportedAssetsToDisc();
	}

	void ResourceManager::OnAssetDeleted(const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(filePath.is_absolute());

		AssetHandle handle = GetAssetHandleFromFilePath(filePath);
		if (handle.IsValid())
			DeleteAsset(handle);
	}

	static AssetMetaData s_NullMetaData;
	AssetMetaData& ResourceManager::GetMetaDataInternal(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();

		if (IsImportedAsset(handle))
			return s_ImportedAssets.at(handle);
		return s_NullMetaData;
	}

	AssetType ResourceManager::GetAssetTypeFormFileExtention(const std::string& fileExtention)
	{
		SK_PROFILE_FUNCTION();

		std::string extention = String::ToLowerCopy(fileExtention);
		if (AssetExtensionMap.find(extention) == AssetExtensionMap.end())
			return AssetType::None;
		return AssetExtensionMap.at(extention);
	}

	void ResourceManager::WriteImportedAssetsToDisc()
	{
		SK_PROFILE_FUNCTION();

		const auto filePath = Project::Directory() / "ImportedAssets.yaml";

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::Value;
		out << YAML::BeginSeq;

		struct Entry
		{
			AssetType Type;
			std::filesystem::path FilePath;
		};

		std::map<UUID, Entry> sortedAssets;
		
		for (const auto& [uuid, metadata] : s_ImportedAssets)
		{
			if (!uuid.IsValid() || metadata.Type == AssetType::None)
				continue;

			const std::filesystem::path filePath = ResourceManager::GetFileSystemPath(metadata);
			if (!std::filesystem::exists(filePath))
				continue;

			auto& entry = sortedAssets[uuid];
			entry.Type = metadata.Type;
			entry.FilePath = metadata.FilePath;
		}


		for (const auto& [handle, entry] : sortedAssets)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << YAML::Hex << handle << YAML::Dec;
			out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(entry.Type);
			out << YAML::Key << "FilePath" << YAML::Value << entry.FilePath;
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

	void ResourceManager::ReadImportedAssetsFromDisc()
	{
		SK_PROFILE_FUNCTION();

		const auto filePath = Project::Directory() / "ImportedAssets.yaml";

		if (!std::filesystem::exists(filePath))
		{
			SK_CORE_ERROR("ImportedAssets File dosn't exist");
			return;
		}

		YAML::Node in = YAML::LoadFile(filePath);

		if (!in["Assets"])
		{
			SK_CORE_ERROR("Invalid ImportedAssets file");
			return;
		}

		s_ImportedAssets.clear();

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

			s_ImportedAssets[metadata.Handle] = metadata;
		}
	}

}
