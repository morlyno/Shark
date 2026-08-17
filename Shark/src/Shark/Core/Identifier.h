#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

namespace Shark {

	struct Identifier
	{
		constexpr Identifier() = default;
		constexpr Identifier(std::string_view id)
			: ID(Hash::ConstexprHash(id)), DebugID(id)
		{
		}

		constexpr Identifier(const char* id)
			: ID(Hash::ConstexprHash(id)), DebugID(id)
		{
		}

		uint64_t ID = 0;
		std::string_view DebugID;

		constexpr bool operator==(const Identifier& other) const { return ID == other.ID; }

		static constexpr Identifier Make(std::string_view id, bool assignDebugID) { Identifier result; result.ID = Hash::ConstexprHash(id); if (assignDebugID) result.DebugID = id; return result; }
	};

}

template<>
struct std::hash<Shark::Identifier>
{
	size_t operator()(const Shark::Identifier& identifier) const
	{
		return identifier.ID;
	}
};
