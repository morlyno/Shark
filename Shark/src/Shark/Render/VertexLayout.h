#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class VertexDataType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Int, Int2, Int3, Int4,
		Bool
	};

	static uint32_t GetVertexTypeSize(VertexDataType type)
	{
		switch (type)
		{
			case VertexDataType::Float:   return 4 * 1;
			case VertexDataType::Float2:  return 4 * 2;
			case VertexDataType::Float3:  return 4 * 3;
			case VertexDataType::Float4:  return 4 * 4;
			case VertexDataType::Int:     return 4 * 1;
			case VertexDataType::Int2:    return 4 * 2;
			case VertexDataType::Int3:    return 4 * 3;
			case VertexDataType::Int4:    return 4 * 4;
			case VertexDataType::Bool:    return 4 * 1;
		}

		SK_CORE_ASSERT(false, "Unknown Element Type");
		return 0;
	}

	struct VertexElement
	{
		std::string Semantic = "";
		VertexDataType Type = VertexDataType::None;
		uint32_t Size = 0;
		uint32_t Offset = 0;

		VertexElement(VertexDataType type, const std::string& semantic)
			: Semantic(semantic), Type(type), Size(GetVertexTypeSize(type))
		{}
	};

	
	class VertexLayout
	{
	public:
		VertexLayout() = default;
		VertexLayout(const std::initializer_list<VertexElement>& elements);
		VertexLayout(std::vector<VertexElement>&& elements);

		void Add(const VertexElement& element);
		void Init();

		uint32_t GetVertexSize();
		uint32_t GetElementCount();

		std::vector<VertexElement>::iterator begin();
		std::vector<VertexElement>::iterator end();

	private:
		void CalcOffsetAndSize();

	private:
		std::vector<VertexElement> m_Elements;
		uint32_t m_VertexSize = 0u;
	};

}
