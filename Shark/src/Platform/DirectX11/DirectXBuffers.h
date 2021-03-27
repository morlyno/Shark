#pragma once

#include "Shark/Render/Buffers.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>

namespace Shark {

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer(const VertexLayout& layout, const Buffer& data, bool dynamic, APIContext apicontext);
		virtual ~DirectXVertexBuffer();

		virtual void SetData(const Buffer& data) override;

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateBuffer(const Buffer& data);
	private:
		ID3D11Buffer* m_VertexBuffer = nullptr;
		bool m_Dynamic;

		APIContext m_APIContext;
	};


	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer(const Buffer& data, bool dynamic, APIContext apicontext);
		virtual ~DirectXIndexBuffer();

		virtual void SetData(const Buffer& data) override;

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		void CreateBuffer(const Buffer& data);
	private:
		ID3D11Buffer* m_IndexBuffer = nullptr;
		bool m_Dynamic;

		APIContext m_APIContext;
	};

}