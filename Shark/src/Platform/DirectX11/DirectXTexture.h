#pragma once

#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXImage.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		//DirectXTexture2D(ID3D11ShaderResourceView* texture, uint32_t width, uint32_t height, const SamplerProps& props = {});
		DirectXTexture2D(Ref<Image2D> image, const SamplerProps& props);
		DirectXTexture2D(const std::filesystem::path& filepath, const SamplerProps& props);
		DirectXTexture2D(uint32_t width, uint32_t height, void* data, const SamplerProps& props);
		virtual ~DirectXTexture2D();

		virtual RenderID GetRenderID() const override { return m_Image->GetViewRenderID(); }
		virtual Ref<Image2D> GetImage() const override { return m_Image; }

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }

		virtual void SetSlot(uint32_t slot) override { m_Slot = slot; }
		virtual uint32_t GetSlot() const override { return m_Slot; }

		virtual void Bind() override { Bind(m_Slot); }
		virtual void UnBind() override { UnBind(m_Slot); }
		virtual void Bind(uint32_t slot) override;
		virtual void UnBind(uint32_t slot) override;

	private:
		void CreateSampler(const SamplerProps& props);

	private:
		Ref<DirectXImage2D> m_Image;

		std::filesystem::path m_FilePath;
		uint32_t m_Slot = 0;
		ID3D11SamplerState* m_Sampler = nullptr;

	};

	class DirectXTexture2DArray : public Texture2DArray
	{
	public:
		DirectXTexture2DArray(uint32_t count);
		virtual ~DirectXTexture2DArray() = default;

		virtual void Resize(uint32_t newCount) override;

		virtual Ref<Texture2D> Create(uint32_t index, Ref<Image2D> image, const SamplerProps& props = {}) override;
		virtual Ref<Texture2D> Create(uint32_t index, const std::filesystem::path& filepath, const SamplerProps& props = {}) override;
		virtual Ref<Texture2D> Create(uint32_t index, uint32_t width, uint32_t height, void* data, const SamplerProps& props = {}) override;

		virtual void Set(uint32_t index, Ref<Texture2D> texture) override;
		virtual Ref<Texture2D> Get(uint32_t index) const override;

		virtual void Bind() override;
		virtual void Bind(uint32_t slot) override;

	private:
		std::vector<Ref<DirectXTexture2D>> m_TextureArray;
	};

}