#pragma once

#include "Shark/Render/Texture.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D(ID3D11ShaderResourceView* texture, uint32_t width, uint32_t height, const SamplerProps& props = {});
		DirectXTexture2D(const SamplerProps& props, const std::filesystem::path& filepath);
		DirectXTexture2D(const SamplerProps& props, uint32_t width, uint32_t height, void* data);
		virtual ~DirectXTexture2D();

		virtual void SetData(void* data) override;
		virtual RenderID GetRenderID() const override { return m_Texture; }

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }
		virtual uint32_t GetSlot() const override { return m_Slot; }

		virtual void Bind() override { Bind(m_Slot); }
		virtual void UnBind() override { UnBind(m_Slot); }
		virtual void Bind(uint32_t slot) override;
		virtual void UnBind(uint32_t slot) override;

		ID3D11ShaderResourceView* GetView() const { return m_Texture; }

	private:
		void CreateTexture(void* data);
		void CreateSampler(const SamplerProps& props);

	private:
		std::filesystem::path m_FilePath;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Slot = 0;
		ID3D11ShaderResourceView* m_Texture = nullptr;
		ID3D11SamplerState* m_Sampler = nullptr;

	};

}