#pragma once

#include "Shark/Render/Texture.h"

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
		bool SwapChainTarget;

		FrmeBufferTextureAtachment(FrameBufferColorAtachment atachment, bool swapchaintarget = false)
			: Atachment(atachment), SwapChainTarget(swapchaintarget) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrmeBufferTextureAtachment> Atachments;

		FrameBufferSpecification() = default;
		FrameBufferSpecification(uint32_t width, uint32_t height, std::initializer_list<FrmeBufferTextureAtachment> atachments)
			: Width(width), Height(height), Atachments(atachments) {}
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Clear(float ClearColor[4]) = 0;
		virtual void ClearAtachment(uint32_t index, float clearcolor[4]) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) = 0;
		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specs);
	};

}