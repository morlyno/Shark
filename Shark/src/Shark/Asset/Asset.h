#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Thread.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	using AssetHandle = UUID;

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Invalid = BIT(0),
		Missing = BIT(1),
		Fallback = BIT(2)
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
				Flags |= flag;
			else
				Flags &= ~flag;
		}

		bool IsFlagSet(AssetFlag flag) const
		{
			return (Flags & flag) == flag;
		}

	public:
		AssetHandle Handle = AssetHandle::Null;
		AssetFlag Flags = AssetFlag::None;
	};

}
