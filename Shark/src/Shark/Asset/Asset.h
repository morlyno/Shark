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

		bool IsValid() const { return Handle.IsValid() && (Type != AssetType::None) && !FilePath.empty(); }
	};

	class Asset : public RefCount
	{
	public:
		Asset() = default;
		virtual ~Asset() = default;

		virtual AssetType GetAssetType() const = 0;
	public:
		AssetHandle Handle;

	};

}
