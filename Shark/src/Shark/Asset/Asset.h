#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	using AssetHandle = UUID;

	struct AssetMetaData
	{
		AssetHandle Handle;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath; // relative to Assets (not Project!)
		bool IsDataLoaded = false;
		bool IsMemoryAsset = false;

		bool IsValid() const { return Handle.IsValid() && (Type != AssetType::None) /*&& (IsMemoryAsset || !FilePath.empty())*/; }
	};

	namespace AssetFlag {
		enum Type : uint16_t
		{
			None = 0,
			Unloaded = BIT(0),
			FileNotFound = BIT(1),
			InvalidFile = BIT(2)
		};
	}
	using AssetFlags = std::underlying_type_t<AssetFlag::Type>;

	class Asset : public RefCount
	{
	public:
		Asset() = default;
		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return GetStaticType(); }

		void SetFlag(AssetFlag::Type flag, bool enabled)
		{
			if (enabled)
				Flags |= flag;
			else
				Flags &= ~flag;
		}

		bool IsOK() const { return Flags == AssetFlag::None; }

	public:
		AssetHandle Handle = AssetHandle::Null;
		AssetFlags Flags = AssetFlag::None;
	};

}
