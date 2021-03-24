#pragma once

#include "Shark/Render/Buffers.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, bool dynamic, APIContext apicontext);
		DirectXVertexBuffer(const VertexLayout& layout, const Buffer& data, bool dynamic, APIContext apicontext);
		~DirectXVertexBuffer();

		void Init(const Buffer& data, bool dynamic);

		void SetData(const Buffer& data) override;
		virtual void UpdateData(const Buffer& data) override;

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11Buffer* m_VertexBuffer = nullptr;
		bool m_Dynamic;

		APIContext m_APIContext;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(const Buffer& data, APIContext apicontext);
		~DirectXIndexBuffer();

		void Init(const Buffer& data);

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;

		APIContext m_APIContext;
	};

}