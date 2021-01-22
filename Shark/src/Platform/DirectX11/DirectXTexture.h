#pragma once

#include "Shark/Render/Texture.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D(const std::string& filepath);
		virtual ~DirectXTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind() override;
	private:
		uint32_t m_Width;
		uint32_t m_Height;
		std::string filepath;
		ID3D11ShaderResourceView* m_Texture = nullptr;
		ID3D11SamplerState* m_Sampler = nullptr;
	};

}