#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	std::string ToString(FilterMode filterMode)
	{
		switch (filterMode)
		{
			case FilterMode::Nearest: return "Nearest";
			case FilterMode::Linear:  return "Linear";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}

	std::string ToString(AddressMode addressMode)
	{
		switch (addressMode)
		{
			case AddressMode::Repeat: return "Repeat";
			case AddressMode::Clamp:  return "Clamp";
			case AddressMode::Mirror: return "Mirror";
			case AddressMode::Border: return "Border";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}


	Ref<Texture2D> Texture2D::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specs, void* data)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(specs, data);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}


	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(format, width, height, data);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::filesystem::path& filePath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(filePath);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2DArray> Texture2DArray::Create(uint32_t count, uint32_t startOffset)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2DArray>::Create(count, startOffset);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}