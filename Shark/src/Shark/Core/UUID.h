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

		operator uint64_t() const { return m_UUID; }
		uint64_t Value() const { return m_UUID; }

		static UUID Generate();
		static constexpr uint64_t Invalid = 0;
	private:
		uint64_t m_UUID = 0;
	};

}

template<>
struct std::hash<Shark::UUID>
{
	size_t operator()(Shark::UUID uuid) const
	{
		return uuid.Value();
	}
};

template<typename Char>
struct fmt::formatter<Shark::UUID, Char> : fmt::formatter<uint64_t, Char>
{
	template<typename FormatContext>
	auto format(const Shark::UUID& uuid, FormatContext& ctx) const -> FormatContext::iterator
	{
		return fmt::formatter<uint64_t, Char>::format(uuid.Value(), ctx);
	}
};
