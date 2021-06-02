#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Utility/Utility.h"

namespace Shark {

	enum class ImageFormat
	{
		None = 0,
		RGBA8,
		R32_SINT,

		Depth32,

		SwapChain,
		Depth = Depth32
	};

	struct FrameBufferAtachment
	{
		bool Blend = false;
		ImageFormat Format;

		FrameBufferAtachment(ImageFormat format)
			: Format(format) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrameBufferAtachment> Atachments;
		DirectX::XMFLOAT4 ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

		FrameBufferSpecification() = default;
		FrameBufferSpecification(uint32_t width, uint32_t height, std::initializer_list<FrameBufferAtachment> atachments)
			: Width(width), Height(height), Atachments(atachments) {}
	};

	class FrameBuffer : public RefCount
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Clear() = 0;
		virtual void Clear(const DirectX::XMFLOAT4& clearcolor) = 0;
		virtual void ClearAtachment(uint32_t index) = 0;
		virtual void ClearAtachment(uint32_t index, const DirectX::XMFLOAT4& clearcolor) = 0;
		virtual void ClearDepth() = 0;

		virtual void Release() = 0;
		virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void SetBlend(uint32_t index, bool blend) = 0;
		virtual bool GetBlend(uint32_t index) const = 0;

		virtual void SetDepth(bool enabled) = 0;
		virtual bool GetDepth() const = 0;

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) = 0;
		virtual int ReadPixel(uint32_t index, int x, int y) = 0;

		virtual const FrameBufferSpecification& GetSpecification() const = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specs);
	};

}