#include "skpch.h"
#include "VertexLayout.h"

namespace Shark {

	VertexLayout::VertexLayout(std::vector<VertexElement>&& elements)
		: m_Elements(std::move(elements))
	{
		CalcOffsetAndSize();
	}

	VertexLayout::VertexLayout(const std::initializer_list<VertexElement>& elements)
		: m_Elements(elements)
	{
		CalcOffsetAndSize();
	}

	VertexLayout::VertexLayout(const VertexElement& element)
	{
		Add(element);
		CalcOffsetAndSize();
	}

	VertexLayout& VertexLayout::operator=(const VertexElement& other)
	{
		m_Elements.clear();
		Add(other);
		CalcOffsetAndSize();
		return *this;
	}

	void VertexLayout::Add(const VertexElement& element)
	{
		m_Elements.emplace_back(element);
	}

	void VertexLayout::Init()
	{
		CalcOffsetAndSize();
	}

	uint32_t VertexLayout::GetVertexSize() const
	{
		return m_VertexSize;
	}

	uint32_t VertexLayout::GetElementCount() const
	{
		return (uint32_t)m_Elements.size();
	}

	std::vector<VertexElement>::iterator VertexLayout::begin()
	{
		return m_Elements.begin();
	}

	std::vector<VertexElement>::iterator VertexLayout::end()
	{
		return m_Elements.end();
	}

	void VertexLayout::CalcOffsetAndSize()
	{
		uint32_t offset = 0u;
		m_VertexSize = 0u;
		for (auto& e : m_Elements)
		{
			e.Offset = offset;
			offset += e.Size;
			m_VertexSize += e.Size;
		}
	}

}
