#pragma once

#include "Shark/Render/Buffers.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, uint32_t size, bool dynamic, Buffer vertexData);
		virtual ~DirectXVertexBuffer();

		virtual void Release() override;

		virtual void Resize(uint32_t size) override;
		virtual void Resize(Buffer vertexData) override;
		virtual void SetData(Buffer vertexData) override;

		virtual Buffer GetWritableBuffer() override;
		virtual void CloseWritableBuffer() override;

		virtual uint32_t GetSize() const { return m_Size; }

	private:
		void ReCreateBuffer(uint32_t size, bool dynamic, Buffer vertexData);

	private:
		VertexLayout m_Layout;

		ID3D11Buffer* m_VertexBuffer = nullptr;
		uint32_t m_Size = 0;
		bool m_Dynamic;

		bool m_Mapped = false;

		friend class DirectXRenderer;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(uint32_t count, bool dynmaic, Buffer indexData);
		virtual ~DirectXIndexBuffer();

		virtual void Release() override;

		virtual void Resize(uint32_t count) override;
		virtual void Resize(Buffer vertexData) override;
		virtual void SetData(Buffer indexData) override;

		virtual Buffer GetWritableBuffer() override;
		virtual void CloseWritableBuffer() override;

		virtual uint32_t GetCount() const override { return m_Count; }
		virtual uint32_t GetSize() const override { return m_Size; }

	private:
		void ReCreateBuffer(uint32_t count, bool dynamic, Buffer indexData);

	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;
		uint32_t m_Size;
		uint32_t m_Count;
		bool m_Dynamic;

		bool m_Mapped = false;

		friend class DirectXRenderer;
	};

}