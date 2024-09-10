#pragma once

#include "Shark/Asset/AssetMetadata.h"
#include <shared_mutex>

namespace Shark {

	class AssetRegistry
	{
	public:
		bool Contains(AssetHandle handle) const;

		void Add(const AssetMetaData& metadata);
		void Update(const AssetMetaData& metadata);
		void Remove(AssetHandle handle);
		void Clear();

		AssetMetaData& Get(AssetHandle handle);
		const AssetMetaData& Read(AssetHandle handle) const;

		AssetHandle TryFind(const std::filesystem::path& assetFilepath) const;

		std::unordered_map<AssetHandle, AssetMetaData>::const_iterator begin() const { return m_Registry.cbegin(); }
		std::unordered_map<AssetHandle, AssetMetaData>::const_iterator end() const { return m_Registry.cend(); }

	private:
		std::unordered_map<AssetHandle, AssetMetaData> m_Registry;
	};

}
