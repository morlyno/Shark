#pragma once

#include "Shark/Utils/String.h"

namespace Shark::UI {

	class TextFilter
	{
	public:
		TextFilter(const std::string& filter);
		bool PassFilter(std::string_view text);
		void SetMode(String::Case compCase) { m_Case = compCase; }

	private:
		void Build();

	private:
		std::string m_Buffer;
		std::vector<std::string_view> m_Filters;
		String::Case m_Case = String::Case::Ingnore;
	};

}
