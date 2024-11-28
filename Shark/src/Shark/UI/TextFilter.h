#pragma once

#include "Shark/Utils/String.h"

namespace Shark::UI {

	class TextFilter
	{
	public:
		TextFilter() = default;
		TextFilter(const std::string& filter);
		bool PassesFilter(std::string_view text) const;
		void SetMode(String::Case compCase) { m_Case = compCase; }
		void EnabelAutoCaseSensitive(bool enabled = true) { m_AutoCaseSensitive = enabled; }

		void Clear();
		void Rebuild();
		void SetFilter(const std::string& filter);

		std::string& GetTextBuffer() { return m_Buffer; }
		const std::string& GetTextBuffer() const { return m_Buffer; }

		operator bool() const { return !m_Filters.empty(); }

	private:
		void Build();

	private:
		std::string m_Buffer;
		std::vector<std::string_view> m_Filters;
		String::Case m_Case = String::Case::Ignore;
		bool m_AutoCaseSensitive = false;

		bool m_HasPositiveFilter = false;
	};

}
