#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Utility/Utils.h"

namespace Shark {

	enum class FrameBufferColorAtachment
	{
		None = 0,
		RGBA8,

		Depth32
	};

	struct FrmeBufferTextureAtachment
	{
		FrameBufferColorAtachment Atachment;

		FrmeBufferTextureAtachment(FrameBufferColorAtachment atachment)
			: Atachment(atachment) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrmeBufferTextureAtachment> Atachments;
		bool SwapChainTarget = false;
		WeakRef<SwapChain> SwapChain = nullptr;

		FrameBufferSpecification() = default;
		FrameBufferSpecification(uint32_t width, uint32_t height, std::initializer_list<FrmeBufferTextureAtachment> atachments)
			: Width(width), Height(height), Atachments(atachments) {}
	};

	class FrameBuffer : public RefCount
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Clear(Utils::ColorF32 clearcolor) = 0;
		virtual void ClearAtachment(uint32_t index, Utils::ColorF32 clearcolor) = 0;

		virtual void Release() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) = 0;
		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specs);
	};

}