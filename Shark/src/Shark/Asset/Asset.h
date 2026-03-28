#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	using AssetHandle = UUID;

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Invalid = BIT(0),
		Missing = BIT(1),
		Fallback = BIT(2),

		AsyncPending = BIT(3)
	};

}

template<>
struct magic_enum::customize::enum_range<Shark::AssetFlag>
{
	static constexpr bool is_flags = true;
};

namespace Shark {

	class Asset : public RefCount
	{
	public:
		Asset() = default;
		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return GetStaticType(); }

		void SetFlag(AssetFlag flag, bool enabled = true)
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
		AssetHandle Handle = AssetHandle::Invalid;
		AssetFlag Flags = AssetFlag::None;
	};

}
