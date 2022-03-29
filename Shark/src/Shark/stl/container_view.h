#pragma once

#include <vector>

namespace stl {

	template<typename Container>
	class container_view
	{
	public:
		using value_type = typename Container::value_type;

		using iterator = typename Container::const_iterator;
		using const_iterator = iterator;

	public:
		container_view()
		{
			m_Begin = const_iterator();
			m_End = const_iterator();
		}

		container_view(const Container& container)
		{
			m_Begin = container.begin();
			m_End = container.end();
		}

		bool empty() const { return m_Begin == m_End; }
		operator bool() const { return !empty(); }

		const_iterator begin() { return m_Begin; }
		const_iterator begin() const { return m_Begin; }
		const_iterator cbegin() { return m_Begin; }
		const_iterator cbegin() const { return m_Begin; }
		
		const_iterator end() { return m_End; }
		const_iterator end() const { return m_End; }
		const_iterator cend() { return m_End; }
		const_iterator cend() const { return m_End; }

	private:
		const_iterator m_Begin;
		const_iterator m_End;
	};

	template<typename T>
	using vector_view = container_view<std::vector<T>>;

}
