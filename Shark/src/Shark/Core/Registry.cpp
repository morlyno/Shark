#include "skpch.h"
#include "Registry.h"
#include <charconv>

namespace Shark {

	bool Registry::IsEmpty() const
	{
		return m_Map.empty();
	}

	bool Registry::HasKey(std::string_view key) const
	{
		return m_Map.contains(key);
	}

	void Registry::RemoveKey(std::string_view key)
	{
		auto it = m_Map.find(key);
		if (it != m_Map.end())
			m_Map.erase(it);
	}

	const std::string& Registry::Get(std::string_view name, const std::string& defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		return m_Map.find(name)->second;
	}

	float Registry::GetFloat(std::string_view name, float defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		return std::stof(m_Map.find(name)->second);
	}

	int Registry::GetInt(std::string_view name, int defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		return std::stoi(m_Map.find(name)->second);
	}

	void Registry::Set(std::string_view name, std::string_view value)
	{
		m_Map[std::string(name)] = value;
	}

	void Registry::SetFloat(std::string_view name, float value)
	{
		m_Map[std::string(name)] = std::to_string(value);
	}

	void Registry::SetInt(std::string_view name, int value)
	{
		m_Map[std::string(name)] = std::to_string(value);
	}

}
