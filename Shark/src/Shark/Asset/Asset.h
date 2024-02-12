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
		bool IsEditorAsset = false;

		SK_DEPRECATED("Replace with AssetManaget::IsAssetHandleValid(metadata.Handle)")
		bool IsValid() const { return Handle != AssetHandle::Invalid && (Type != AssetType::None) /*&& (IsMemoryAsset || !FilePath.empty())*/; }
	};

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Invalid = BIT(0)
	};


	class Asset : public RefCount
	{
	public:
		Asset() = default;
		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return GetStaticType(); }

		void SetFlag(AssetFlag flag, bool enabled)
		{
			if (enabled)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}

		bool IsFlagSet(AssetFlag flag) const
		{
			return Flags & (uint16_t)flag;
		}

	public:
		AssetHandle Handle = AssetHandle::Null;
		uint16_t Flags = (uint16_t)AssetFlag::None;
	};

}
