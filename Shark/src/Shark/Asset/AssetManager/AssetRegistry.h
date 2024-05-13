#pragma once

#include "Shark/Asset/AssetMetadata.h"

namespace Shark {

	class AssetRegistry
	{
	public:
		bool Has(AssetHandle handle) const;
		AssetMetaData& Get(AssetHandle handle);
		const AssetMetaData& Get(AssetHandle handle) const;
		AssetMetaData& TryGet(AssetHandle handle);
		const AssetMetaData& TryGet(AssetHandle handle) const;
		AssetMetaData& operator[](AssetHandle handle);
		void Remove(AssetHandle handle);
		void Clear();

		const AssetMetaData& Find(const std::filesystem::path& assetFilepath) const;

		std::unordered_map<AssetHandle, AssetMetaData>::iterator begin() { return m_Registry.begin(); }
		std::unordered_map<AssetHandle, AssetMetaData>::iterator end() { return m_Registry.end(); }

	private:
		std::unordered_map<AssetHandle, AssetMetaData> m_Registry;
	};

}
