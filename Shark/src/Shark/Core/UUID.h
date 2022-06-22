#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

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

		bool operator<(const UUID& rhs) const { return m_UUID < rhs.m_UUID; }

		static UUID Generate();
		static constexpr uint64_t Null = 0;
	private:
		uint64_t m_UUID = 0;

		friend struct std::hash<UUID>;
	};

}

namespace std {

	template<>
	struct hash<Shark::UUID>
	{
		size_t operator()(Shark::UUID uuid) const
		{
			return ::Shark::Hash::FNV1A(uuid.m_UUID);
		}
	};

}
