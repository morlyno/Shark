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

		SK_DEPRECATED("use uuid != UUID::Null or uuid != UUID::Invalid instead")
		bool IsValid() const;
		operator uint64_t() const { return m_UUID; }

		//bool operator<(const UUID& rhs) const { return m_UUID < rhs.m_UUID; }
		//bool operator==(const UUID& rhs) const { return m_UUID == rhs.m_UUID; }
		//bool operator!=(const UUID& rhs) const { return !(*this == rhs); }

		static UUID Generate();
		static constexpr uint64_t Null = 0;
		enum { Invalid = 0 };
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
			return (uint64_t)uuid;
		}
	};

}
