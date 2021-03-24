#pragma once

#include "Shark/Render/Texture.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D(const SamplerSpecification& specs, const std::string& filepath, APIContext apicontext);
		DirectXTexture2D(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t flatcolor, APIContext apicontext);
		DirectXTexture2D(const SamplerSpecification& specs, uint32_t width, uint32_t height, const Buffer& data, APIContext apicontext);
		virtual ~DirectXTexture2D();

		virtual void SetData(const Buffer& data) override;
		virtual void* GetHandle() const override { return m_Texture; }

		virtual const std::string& GetFilePath() const override { return m_FilePath; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }
		virtual uint32_t GetSlot() const override { return m_Slot; }

		virtual void Bind() override;
		virtual void Bind(uint32_t slot) override;
	private:
		std::string m_FilePath;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Slot = 0;
		ID3D11ShaderResourceView* m_Texture = nullptr;
		ID3D11SamplerState* m_Sampler = nullptr;

		APIContext m_APIContext;
	};

}