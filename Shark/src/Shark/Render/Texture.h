#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual const std::string& GetName() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetSlot(uint32_t slot) = 0;

		virtual void Bind() = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		static Ref<Texture2D> Create(const std::string& filepath);
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, uint32_t color, const std::string& name = std::string());
	};

}