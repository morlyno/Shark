#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class UUID
	{
	public:
		UUID() = default;
		UUID(const UUID&) = default;
		UUID(uint64_t uuid);

		bool IsValid() const;
		operator const uint64_t() const { return m_UUID; }
		operator uint64_t() { return m_UUID; }

		static UUID Generate();
		static UUID Null();
	private:
		uint64_t m_UUID = 0;
	};

}

namespace std {

	template<>
	struct hash<Shark::UUID>
	{
		size_t operator()(Shark::UUID uuid) const
		{
			return hash<uint64_t>{}(uuid);
		}
	};

}
