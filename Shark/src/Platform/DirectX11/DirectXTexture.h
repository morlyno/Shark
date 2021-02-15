#pragma once

#include "Shark/Render/Texture.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D(const std::string& filepath);
		DirectXTexture2D(uint32_t width, uint32_t height, uint32_t color, const std::string& name);
		virtual ~DirectXTexture2D();

		virtual const std::string& GetName() const override { return m_Name; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetSlot(uint32_t slot = 0) override { m_Slot = slot; }

		virtual void Bind() override;
	private:
		std::string m_Name;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Slot = 0;
		ID3D11ShaderResourceView* m_Texture = nullptr;
		ID3D11SamplerState* m_Sampler = nullptr;
	};

}