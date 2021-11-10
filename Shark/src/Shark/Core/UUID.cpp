#include "skpch.h"
#include "UUID.h"

#include <random>

namespace Shark {

	static std::random_device s_Device;
	static std::mt19937_64 s_Engine{ s_Device() };
	static std::uniform_int_distribution<uint64_t> s_Distribution;

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{
	}

	bool UUID::Valid() const
	{
		return m_UUID != 0;
	}

	UUID UUID::Create()
	{
		UUID uuid{ s_Distribution(s_Engine) };
		while (!uuid.Valid())
			uuid = s_Distribution(s_Engine);
		return uuid;
	}

	UUID UUID::Null()
	{
		return UUID(0);
	}

}