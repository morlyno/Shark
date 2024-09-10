#include "skpch.h"
#include "AssetRegistry.h"

namespace Shark {

	bool AssetRegistry::Contains(AssetHandle handle) const
	{
		return m_Registry.contains(handle);
	}

	void AssetRegistry::Add(const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(!m_Registry.contains(metadata.Handle));
		m_Registry.emplace(metadata.Handle, metadata);
	}

	void AssetRegistry::Update(const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(m_Registry.contains(metadata.Handle));
		m_Registry.at(metadata.Handle) = metadata;
	}

	void AssetRegistry::Remove(AssetHandle handle)
	{
		SK_CORE_VERIFY(m_Registry.contains(handle));
		m_Registry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		m_Registry.clear();
	}

	AssetMetaData& AssetRegistry::Get(AssetHandle handle)
	{
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	const AssetMetaData& AssetRegistry::Read(AssetHandle handle) const
	{
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	AssetHandle AssetRegistry::TryFind(const std::filesystem::path& assetFilepath) const
	{
		for (const auto& [handle, metadata] : m_Registry)
			if (metadata.FilePath == assetFilepath)
				return metadata.Handle;
		return AssetHandle::Invalid;
	}

}
