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

	ResourceManager::ResourceManagerData* ResourceManager::s_Data = nullptr;

	void ResourceManager::Init()
	{
		s_Data = new ResourceManagerData;
		AssetSerializer::RegisterSerializers();
		ReadImportedAssetsFromDisc();
	}

	void ResourceManager::Shutdown()
	{
		WriteImportedAssetsToDisc();
		AssetSerializer::ReleaseSerializers();
		delete s_Data;
		s_Data = nullptr;
	}

	const AssetMetaData& ResourceManager::GetMetaData(AssetHandle handle)
	{
		return GetMetaDataInternal(handle);
	}

	AssetType ResourceManager::GetAssetTypeFormFilePath(const std::filesystem::path& filePath)
	{
		return GetAssetTypeFormExtension(filePath.extension().string());
	}

	AssetType ResourceManager::GetAssetTypeFormExtension(const std::string& fileExtension)
	{
		std::string extention = String::ToLowerCopy(fileExtension);
		if (AssetExtensionMap.find(extention) == AssetExtensionMap.end())
			return AssetType::None;
		return AssetExtensionMap.at(extention);
	}

	bool ResourceManager::IsValidAssetHandle(AssetHandle handle)
	{
		return s_Data->ImportedAssets.find(handle) != s_Data->ImportedAssets.end();
	}

	bool ResourceManager::IsMemoryAsset(AssetHandle handle)
	{
const auto& metadata = GetMetaDataInternal(handle);
return metadata.IsMemoryAsset;
	}

	bool ResourceManager::IsFileImported(const std::filesystem::path& path)
	{
		return GetAssetHandleFromFilePath(path).IsValid();
	}

	bool ResourceManager::HasExistingFilePath(const AssetMetaData& metadata)
	{
		return std::filesystem::exists(GetFileSystemPath(metadata));
	}

	std::filesystem::path ResourceManager::MakeRelativePath(const std::filesystem::path& filePath)
	{
		auto relativePath = std::filesystem::relative(Project::GetAssetsPath() / filePath, Project::GetAssetsPath());
		String::FormatDefault(relativePath);
		return relativePath;
	}

	std::filesystem::path ResourceManager::GetFileSystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		std::filesystem::path fsPath = Project::GetAssetsPath() / metadata.FilePath;
		return fsPath.lexically_normal();
	}

	std::filesystem::path ResourceManager::GetProjectPath(const AssetMetaData& metadata)
	{
		return Project::RelativeCopy(GetFileSystemPath(metadata));
	}

	AssetHandle ResourceManager::GetAssetHandleFromFilePath(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		auto relativePath = MakeRelativePath(filePath);
		for (const auto& [handle, metadata] : s_Data->ImportedAssets)
			if (metadata.FilePath == relativePath)
				return metadata.Handle;
		return AssetHandle();
	}

	bool ResourceManager::LoadAsset(AssetHandle handle)
	{
		auto& metadata = GetMetaDataInternal(handle);
		if (!metadata.IsDataLoaded && !metadata.IsMemoryAsset)
		{
			Ref<Asset> asset = nullptr;
			metadata.IsDataLoaded = AssetSerializer::TryLoadData(asset, metadata);
			asset->Handle = handle;
			s_Data->LoadedAssets[handle] = asset;
		}
		return metadata.IsDataLoaded;
	}

	bool ResourceManager::SaveAsset(AssetHandle handle)
	{
		const auto& metadata = GetMetaDataInternal(handle);
		if (metadata.IsMemoryAsset || !metadata.IsDataLoaded)
			return false;

		return AssetSerializer::Serialize(s_Data->LoadedAssets.at(handle), metadata);
	}

	void ResourceManager::ReloadAsset(AssetHandle handle)
	{
		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (metadata.IsMemoryAsset)
			return;

		if (metadata.IsDataLoaded)
		{
			Ref<Asset> asset = s_Data->LoadedAssets.at(handle);
			SK_CORE_ASSERT(asset);
			asset->SetFlag(AssetFlag::Unloaded, true);
			UnloadAsset(handle);
		}
		LoadAsset(handle);
	}

	void ResourceManager::UnloadAsset(AssetHandle handle)
	{
		SK_CORE_ASSERT(IsValidAssetHandle(handle));

		AssetMetaData& metadata = GetMetaDataInternal(handle);
		SK_CORE_ASSERT(metadata.IsDataLoaded == (s_Data->LoadedAssets.find(handle) != s_Data->LoadedAssets.end()));
		if (!metadata.IsDataLoaded)
			return;

		utils::SetFlag(handle, AssetFlag::Unloaded, true);

		metadata.IsDataLoaded = false;
		s_Data->LoadedAssets.erase(handle);

		if (metadata.IsMemoryAsset)
			s_Data->ImportedAssets.erase(handle);
	}

	void ResourceManager::RemoveAsset(AssetHandle handle)
	{
		const bool isMemoryAsset = GetMetaDataInternal(handle).IsMemoryAsset;

		s_Data->LoadedAssets.erase(handle);
		s_Data->ImportedAssets.erase(handle);

		if (!isMemoryAsset)
			WriteImportedAssetsToDisc();
	}

	bool ResourceManager::ImportMemoryAsset(AssetHandle handle, const std::string& directoryPath, const std::string& fileName)
	{
		AssetMetaData& metadata = GetMetaDataInternal(handle);
		if (!metadata.IsValid())
			return false;

		metadata.FilePath = MakeRelativePath(directoryPath + "/" + fileName);
		metadata.IsMemoryAsset = false;

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

		Ref<Asset> asset = s_Data->LoadedAssets.at(handle);
		if (!AssetSerializer::Serialize(asset, metadata))
		{
			metadata.FilePath.clear();
			metadata.IsMemoryAsset = true;
			return false;
		}

		WriteImportedAssetsToDisc();
		return true;
	}

	AssetHandle ResourceManager::ImportAsset(const std::filesystem::path& filePath)
	{
		AssetType type = GetAssetTypeFormFilePath(filePath);
		if (type == AssetType::None)
			return 0;

		if (!std::filesystem::exists(Project::GetAssetsPath() / filePath))
			return 0;

		AssetHandle handle = GetAssetHandleFromFilePath(filePath);
		if (handle.IsValid())
			return handle;


		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.FilePath = MakeRelativePath(filePath);
		metadata.Type = type;
		metadata.IsDataLoaded = false;
		s_Data->ImportedAssets[metadata.Handle] = metadata;
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("ResourceManager", "Imported Asset => Handle: 0x{:x}, Type: {}, FilePath: {}", metadata.Handle, ToString(metadata.Type), metadata.FilePath);
		return metadata.Handle;
	}

	void ResourceManager::OnFileEvents(const std::vector<FileChangedData>& fileEvents)
	{
		std::filesystem::path oldFilePath;
		for (const FileChangedData& event : fileEvents)
		{
			if (event.Type == FileEvent::NewName && !oldFilePath.empty())
			{
				OnAssetRenamed(oldFilePath, event.FilePath);
				break;
			}

			AssetType assetType = GetAssetTypeFormFilePath(event.FilePath);
			if (assetType != AssetType::None)
			{
				switch (event.Type)
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
			RemoveAsset(handle);
	}

	static AssetMetaData s_NullMetaData;
	AssetMetaData& ResourceManager::GetMetaDataInternal(AssetHandle handle)
	{
		if (s_Data->ImportedAssets.find(handle) != s_Data->ImportedAssets.end())
			return s_Data->ImportedAssets.at(handle);
		return s_NullMetaData;
	}

	void ResourceManager::WriteImportedAssetsToDisc()
	{
		const auto filePath = Project::GetDirectory() / "ImportedAssets.yaml";

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::Value;
		out << YAML::BeginSeq;

		struct Entry
		{
			AssetHandle Handle;
			AssetType Type;
			std::filesystem::path FilePath;

			bool operator<(const Entry& rhs) const
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

		std::set<Entry> sortedAssets;
		
		for (const auto& [assetHandle, metadata] : s_Data->ImportedAssets)
		{
			if (!metadata.IsValid() || metadata.IsMemoryAsset)
				continue;

			const std::filesystem::path filePath = ResourceManager::GetFileSystemPath(metadata);
			if (!std::filesystem::exists(filePath))
				continue;

			sortedAssets.emplace(Entry{ metadata.Handle, metadata.Type, metadata.FilePath });
		}


		for (const auto& entry : sortedAssets)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << YAML::Hex << entry.Handle << YAML::Dec;
			out << YAML::Key << "Type" << YAML::Value << ToString(entry.Type);
			out << YAML::Key << "FilePath" << YAML::Value << entry.FilePath;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filePath);
		if (!fout)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to write imported assets to disc");
			return;
		}

		fout << out.c_str();
		fout.close();
	}

	void ResourceManager::ReadImportedAssetsFromDisc()
	{
		SK_PROFILE_FUNCTION();

		const auto filePath = Project::GetDirectory() / "ImportedAssets.yaml";

		if (!std::filesystem::exists(filePath))
		{
			SK_CORE_ERROR_TAG("Serialization", "ImportedAssets.yaml not found");
			return;
		}

		YAML::Node in = YAML::LoadFile(filePath);

		if (!in["Assets"])
		{
			SK_CORE_ERROR_TAG("Serialization", "Invalid ImportedAssets file");
			return;
		}

		s_Data->ImportedAssets.clear();

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

			if (!ResourceManager::HasExistingFilePath(metadata))
				continue;

			s_Data->ImportedAssets[metadata.Handle] = metadata;
		}
	}

}
