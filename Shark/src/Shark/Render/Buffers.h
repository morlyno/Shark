#pragma once

#include "Shark/Core/Core.h"

namespace Shark {

	enum class VertexElementType
	{
		None = 0,
		Float,Float2,Float3,Float4,
		Int,Int2,Int3,Int4,
	};

	static uint32_t GetElementSize( VertexElementType type )
	{
		switch ( type )
		{
			case VertexElementType::Float:   return 4u;
			case VertexElementType::Float2:  return 4u * 2u;
			case VertexElementType::Float3:  return 4u * 3u;
			case VertexElementType::Float4:  return 4u * 4u;
			case VertexElementType::Int:     return 4u;
			case VertexElementType::Int2:    return 4u * 2u;
			case VertexElementType::Int3:    return 4u * 3u;
			case VertexElementType::Int4:    return 4u * 4u;
		}

		SK_CORE_ASSERT( false,"Unknown Element Type" );
		return 0u;
	}

	struct ElementDesc
	{
		std::string name;
		uint32_t size;
		uint32_t offset;
		VertexElementType type;

		ElementDesc( VertexElementType type,const std::string& name )
			: type( type ),name( name ),size( GetElementSize( type ) ),offset( 0u ) {}

		ElementDesc() = default;
	};

	class VertexLayout
	{
	public:
		VertexLayout( std::initializer_list<ElementDesc> elements )
			: m_Elements( elements )
		{
			CalcOffsetAndStride();
		}

		uint32_t GetVertexSize() { return m_VertexSize; }
		uint32_t GetElementCount() { return (uint32_t)m_Elements.size(); }

		std::vector<ElementDesc>::iterator begin() { return m_Elements.begin(); }
		std::vector<ElementDesc>::iterator end() { return m_Elements.end(); }

	private:
		void CalcOffsetAndStride()
		{
			uint32_t offset = 0u;
			m_VertexSize = 0u;
			for ( auto& e : m_Elements )
			{
				e.offset = offset;
				offset += e.size;
				m_VertexSize += e.size;
			}
		}
	private:
		std::vector<ElementDesc> m_Elements;
		uint32_t m_VertexSize = 0u;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer( const VertexLayout& layout )
			: m_Layout( layout ) {}
		VertexBuffer( const VertexLayout& layout,void* data,uint32_t count )
			: m_Layout( layout ),m_Data( data ),m_Count( count ) {}
		virtual ~VertexBuffer() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void SetData( void* data,uint32_t count ) = 0;

		const VertexLayout& GetLayout() { return m_Layout; }

		static VertexBuffer* Create( const VertexLayout& layout );
		static VertexBuffer* Create( const VertexLayout& layout,float* data,uint32_t count );
	protected:
		VertexLayout m_Layout;
		uint32_t m_Count = 0u;
		void* m_Data = nullptr;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer( uint32_t* indices,uint32_t count )
			: m_Indices( indices ),m_Count( count ) {}
		virtual ~IndexBuffer() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		uint32_t GetCount() { return m_Count; }

		static IndexBuffer* Create( uint32_t* indices,uint32_t count );
	private:
		uint32_t m_Count;
		uint32_t* m_Indices = nullptr;
	};

}