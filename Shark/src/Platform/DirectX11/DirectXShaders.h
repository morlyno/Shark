#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Render/Shaders.h"
#include <d3d11.h>

namespace Shark {

	class DirectXShaders : public Shaders
	{
	public:
		DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
		DirectXShaders(VertexLayout& layout, const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
		~DirectXShaders();

		void Init(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);

		void SetInputs(VertexLayout& layout) override;

		virtual void SetSceanData(ShaderType target, uint32_t slot, void* data, uint32_t size) override;

		void Bind() override;
		void UnBind() override;
	private:
		ID3D11VertexShader* m_VertexShader = nullptr;
		ID3D11PixelShader* m_PixelShader = nullptr;
		ID3D11InputLayout* m_InputLayout = nullptr;

		ID3DBlob* m_VSBlob = nullptr;

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputElements;
	};

}