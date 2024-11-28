#include "skpch.h"
#include "TextFilter.h"

#include "Shark/Utils/String.h"

namespace Shark::UI {

	TextFilter::TextFilter(const std::string& filter)
		: m_Buffer(filter)
	{
		Build();
	}

	bool TextFilter::PassesFilter(std::string_view text) const
	{
		if (m_Filters.empty())
			return true;

		for (std::string_view filter : m_Filters)
		{
			if (filter.empty())
				return true;

			const char first = filter.front();

			if (first == '-')
			{
				if (String::Contains(text, filter.substr(1), m_Case == String::Case::Sensitive))
					return false;
				continue;
			}

			if (first == '.')
			{
				if (!String::StartsWith(text, filter.substr(1), m_Case))
					return false;
				continue;
			}

			const char last = filter.back();
			if (last == '.')
			{
				if (!String::EndsWith(text, filter.substr(0, filter.size() - 1), m_Case))
					return false;
				continue;
			}

			if (!String::Contains(text, filter, m_Case == String::Case::Sensitive))
				return false;
		}

		return true;
	}

	void TextFilter::Build()
	{
		m_Filters.clear();
		String::SplitToRanges(m_Buffer, " ", m_Filters);

		if (m_AutoCaseSensitive)
			m_Case = String::Case::Ignore;

		m_HasPositiveFilter = false;
		for (std::string_view filter : m_Filters)
		{
			if (m_AutoCaseSensitive)
			{
				for (auto c : filter)
				{
					if (!std::isupper(c))
						continue;

					m_Case = String::Case::Sensitive;
					break;
				}
			}

			if (filter.front() != '-')
				m_HasPositiveFilter = true;

			if (m_HasPositiveFilter && m_Case == String::Case::Sensitive)
				break;
		}
	}

	void TextFilter::SetFilter(const std::string& filter)
	{
		m_Buffer = filter;
		Build();
	}

	void TextFilter::Rebuild()
	{
		Build();
	}

	void TextFilter::Clear()
	{
		m_Filters.clear();
		m_Buffer.clear();
	}

}
