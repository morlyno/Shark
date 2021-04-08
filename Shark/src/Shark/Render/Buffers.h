#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Utility/Buffer.h"
#include "Shark/Render/VertexLayout.h"

namespace Shark {

	class VertexBuffer : public RefCount
	{
	public:
		VertexBuffer(const VertexLayout& layout)
			: m_Layout(layout) {}
		virtual ~VertexBuffer() = default;

		virtual void SetData(const Buffer& data) = 0;

		virtual uint32_t GetSize() const = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<VertexBuffer> Create(const VertexLayout& layout, const Buffer& data = {}, bool dynamic = false);
	protected:
		VertexLayout m_Layout;
	};

	// 32-Bit IndexBuffer
	class IndexBuffer : public RefCount
	{
	public:
		using IndexType = uint32_t;
	public:
		IndexBuffer(uint32_t count)
			: m_Count(count) {}
		virtual ~IndexBuffer() = default;

		virtual void SetData(const Buffer& data) = 0;

		uint32_t GetCount() { return m_Count; }

		virtual uint32_t GetSize() const = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<IndexBuffer> Create(const Buffer& data, bool dynamic = false);
	protected:
		uint32_t m_Count;
	};

}