#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"
#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	struct Buffer
	{
		ID3D11Buffer* buffer;
		UINT size;
		UINT slot;
	};

	struct PixelShader
	{
		ID3D11PixelShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::unordered_map<std::string, Buffer> constBuffers;
	};

	struct VertexShader
	{
		ID3D11VertexShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::unordered_map<std::string, Buffer> constBuffers;
	};

	class DirectXShaders : public Shaders
	{
	public:
		DirectXShaders(const std::string& filepath);
		DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
		~DirectXShaders();

		void Init(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);

		virtual void SetBuffer(const std::string& bufferName, void* data, uint32_t size) override;

		void UploudBuffer(const std::string& bufferName, void* data, uint32_t size);

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		void Bind() override;
		void UnBind() override;

		virtual const std::string& GetName() override { return m_Name; }
	private:
		PixelShader m_PixelShader;
		VertexShader m_VertexShader;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		VertexLayout m_VertexLayout;

		std::string m_Name;
	};

}