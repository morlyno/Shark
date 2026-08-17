#include "skpch.h"
#include "Properties.h"

namespace Shark::NodeGraph {

	bool Properties::IsEmpty() const
	{
		return m_Properties.empty();
	}

	size_t Properties::Size() const
	{
		return m_Properties.size();
	}

	bool Properties::HasValue(std::string_view name) const
	{
		for (auto& property : m_Properties)
			if (property.Name == name)
				return true;
		return false;
	}

	void Properties::Remove(std::string_view name)
	{
		std::erase_if(m_Properties, [name](auto& property) { return property.Name == name; });
	}

	choc::value::ValueView Properties::GetValue(std::string_view name) const
	{
		for (auto& property : m_Properties)
			if (property.Name == name)
				return property.Value;
		return {};
	}

	void Properties::Set(std::string_view name, choc::value::Value value)
	{
		for (auto& property : m_Properties)
		{
			if (property.Name == name)
			{
				property.Value = std::move(value);
				return;
			}
		}

		m_Properties.push_back({ std::string(name), std::move(value) });
	}

	void Properties::Set(std::string_view name, choc::value::ValueView value)
	{
		Set(name, choc::value::Value(value));
	}

}
