#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	class UUID
	{
		struct Invalid{};
	public:
		constexpr UUID() = default;
		constexpr UUID(const UUID&) = default;
		constexpr UUID(Invalid) {};

		constexpr operator uint64_t() const { return m_UUID; }
		constexpr uint64_t Value() const { return m_UUID; }

		constexpr bool operator==(UUID other) const { return m_UUID == other.m_UUID; }
		constexpr bool operator!=(UUID other) const { return !(*this == other); }

		static UUID Generate();
		static constexpr UUID Make(uint64_t uuid) { return UUID(uuid); }
		static constexpr Invalid Invalid;

	private:
		explicit constexpr UUID(uint64_t uuid);

		uint64_t m_UUID = 0;

		friend struct YAML::convert<UUID>;
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

template<>
struct YAML::convert<Shark::UUID>
{
	static YAML::Node encode(const Shark::UUID& uuid)
	{
		return YAML::convert<uint64_t>::encode(uuid);
	}

	static bool decode(const YAML::Node& node, Shark::UUID& uuid)
	{
		return YAML::convert<uint64_t>::decode(node, uuid.m_UUID);
	}
};
