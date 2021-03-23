#pragma once

#include "Shark/Core/Base.h"

#include <DirectXMath.h>

namespace Shark {

	enum class FilterMode { Linera, Point };
	enum class AddressMode { Wrap, Clamp, Border };

	struct SamplerSpecification
	{
		FilterMode MinMag;
		FilterMode Mipmap;
		AddressMode AddressU;
		AddressMode AddressV;
		AddressMode AddressW;
		DirectX::XMFLOAT4 BorderColor;

		SamplerSpecification(FilterMode minmag = FilterMode::Point, FilterMode mipmap = FilterMode::Point,
			AddressMode u = AddressMode::Wrap, AddressMode v = AddressMode::Wrap, AddressMode w = AddressMode::Wrap,
			const DirectX::XMFLOAT4& bordercolor = { 0.0f, 0.0f, 0.0f, 0.0f })
			: MinMag(minmag), Mipmap(mipmap), AddressU(u), AddressV(v), AddressW(w), BorderColor(bordercolor)
		{}
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual void SetData(void* data) = 0;
		virtual void* GetHandle() const = 0;

		virtual const std::string& GetFilePath() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetSlot(uint32_t slot) = 0;
		virtual uint32_t GetSlot() const = 0;

		virtual void Bind() = 0;
		virtual void Bind(uint32_t slot) = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		static Ref<Texture2D> Create(const SamplerSpecification& sampler, const std::string& filepath);
		static Ref<Texture2D> Create(const SamplerSpecification& sampler, uint32_t width, uint32_t height, uint32_t flatcolor);
		static Ref<Texture2D> Create(const SamplerSpecification& sampler, uint32_t width, uint32_t height, void* data);
	};

}