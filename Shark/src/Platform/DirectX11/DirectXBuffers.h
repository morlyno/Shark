#pragma once

#include "Shark/Render/Buffers.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic);
		virtual ~DirectXVertexBuffer();

		virtual void Resize(uint32_t size) override;
		virtual void SetData(void* data, uint32_t size) override;

		virtual uint32_t GetSize() const { return m_Size; }

		virtual void Bind() override;
		virtual void UnBind() override;

	private:
		void CreateBuffer(void* data, uint32_t size);

	private:
		VertexLayout m_Layout;

		ID3D11Buffer* m_VertexBuffer = nullptr;
		uint32_t m_Size = 0;
		bool m_Dynamic;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(IndexType* data, uint32_t count, bool dynamic);
		virtual ~DirectXIndexBuffer();

		virtual void Resize(uint32_t count) override;
		virtual void SetData(IndexType* data, uint32_t count) override;

		virtual uint32_t GetCount() const override { return m_Count; }
		virtual uint32_t GetSize() const override { return m_Size; }

		virtual void Bind() override;
		virtual void UnBind() override;

	private:
		void CreateBuffer(IndexType* data, uint32_t count);

	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;
		uint32_t m_Count;
		uint32_t m_Size;
		bool m_Dynamic;
	};

}