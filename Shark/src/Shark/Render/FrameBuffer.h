#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/SwapChain.h"
#include "Shark/Render/Shaders.h"
#include "Shark/Utility/Utils.h"
#include <any>

namespace Shark {

	enum class FrameBufferColorAtachment
	{
		None = 0,
		RGBA8,
		R32_SINT,

		Depth32
	};

	struct FrmeBufferTextureAtachment
	{
		FrameBufferColorAtachment Atachment;
		bool Blend = false;
		// TODO: Blend Desc

		FrmeBufferTextureAtachment(FrameBufferColorAtachment atachment)
			: Atachment(atachment) {}
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		std::vector<FrmeBufferTextureAtachment> Atachments;
		bool SwapChainTarget = false;
		WeakRef<SwapChain> SwapChain = nullptr;
		Ref<Shaders> ClearShader = nullptr;

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

		virtual bool HasClearShader() const = 0;
		virtual const Ref<Shaders>& GetClearShader() const = 0;
		virtual void SetClearShader(const Ref<Shaders>& clearshader) = 0;

		virtual void Release() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void SetBlend(uint32_t index, bool blend) = 0;
		virtual bool GetBlend(uint32_t index) const = 0;

		virtual void SetDepth(bool enabled) = 0;

		virtual Ref<Texture2D> GetFramBufferContent(uint32_t index) = 0;
		virtual void GetFramBufferContent(uint32_t index, const Ref<Texture2D>& texture) = 0;
		virtual int ReadPixel(uint32_t index, int x, int y) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& specs);
	};

}