#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Image.h"

#include <DirectXMath.h>

namespace Shark {

	enum class FilterMode { Linera, Point };
	enum class AddressMode { Wrap, Clamp, Border };

	struct SamplerProps
	{
		FilterMode MinMag;
		FilterMode Mipmap;
		AddressMode AddressU;
		AddressMode AddressV;
		AddressMode AddressW;
		DirectX::XMFLOAT4 BorderColor;

		SamplerProps(FilterMode minmag = FilterMode::Point, FilterMode mipmap = FilterMode::Point,
			AddressMode u = AddressMode::Wrap, AddressMode v = AddressMode::Wrap, AddressMode w = AddressMode::Wrap,
			const DirectX::XMFLOAT4& bordercolor = { 0.0f, 0.0f, 0.0f, 0.0f })
			: MinMag(minmag), Mipmap(mipmap), AddressU(u), AddressV(v), AddressW(w), BorderColor(bordercolor)
		{}
	};

	class Texture : public RefCount
	{
	public:
		virtual ~Texture() = default;

		virtual RenderID GetRenderID() const = 0;

		virtual const std::filesystem::path& GetFilePath() const = 0;

		virtual void SetSlot(uint32_t slot) = 0;
		virtual uint32_t GetSlot() const = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void Bind(uint32_t slot) = 0;
		virtual void UnBind(uint32_t slot) = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		virtual Ref<Image2D> GetImage() const = 0;

		static Ref<Texture2D> Create(Ref<Image2D> image, const SamplerProps& props = {});
		static Ref<Texture2D> Create(const std::filesystem::path& filepath, const SamplerProps& props = {});
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, void* data, const SamplerProps& props = {});
	};

}