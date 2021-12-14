#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	class AssetRegistry
	{
	public:
		AssetRegistry();
		~AssetRegistry();

		AssetMetaData& operator[](AssetHandle handle);
		AssetMetaData& Get(AssetHandle handle);
		const AssetMetaData& Get(AssetHandle handle) const;

		void Remove(AssetHandle handle);
		bool Contains(AssetHandle handle) const;
		void Clear();
		uint32_t Count() const;

		std::unordered_map<AssetHandle, AssetMetaData>::iterator begin() { return m_AssetMap.begin(); }
		std::unordered_map<AssetHandle, AssetMetaData>::iterator end() { return m_AssetMap.end(); }
		std::unordered_map<AssetHandle, AssetMetaData>::const_iterator begin() const { return m_AssetMap.begin(); }
		std::unordered_map<AssetHandle, AssetMetaData>::const_iterator end() const { return m_AssetMap.end(); }

	private:
		std::unordered_map<AssetHandle, AssetMetaData> m_AssetMap;
	};

}
