#pragma once

#include "Shark/Core/Base.h"
#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {

	class Properties
	{
	public:
		struct Property
		{
			std::string Name;
			choc::value::Value Value;
		};

	public:
		bool IsEmpty() const;
		size_t Size() const;

		bool HasValue(std::string_view name) const;
		void Remove(std::string_view name);

		choc::value::ValueView GetValue(std::string_view name) const;
		void Set(std::string_view name, choc::value::Value value);
		void Set(std::string_view name, choc::value::ValueView value);

		auto GetNames() const { return m_Properties | std::views::transform(&Property::Name); }
	private:
		std::vector<Property> m_Properties;
	};

}
