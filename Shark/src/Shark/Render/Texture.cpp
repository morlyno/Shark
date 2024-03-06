#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	std::string ToString(FilterMode filterMode)
	{
		switch (filterMode)
		{
			case FilterMode::None: return "None";
			case FilterMode::Nearest: return "Nearest";
			case FilterMode::Linear:  return "Linear";
			case FilterMode::Anisotropic: return "Anisotropic";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}

	std::string ToString(WrapMode wrapMode)
	{
		switch (wrapMode)
		{
			case WrapMode::None: return "None";
			case WrapMode::Repeat: return "Repeat";
			case WrapMode::Clamp:  return "Clamp";
			case WrapMode::Mirror: return "Mirror";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}
	
	std::string_view ToStringView(FilterMode filterMode)
	{
		switch (filterMode)
		{
			case FilterMode::None: return "None"sv;
			case FilterMode::Nearest: return "Nearest"sv;
			case FilterMode::Linear: return "Linear"sv;
			case FilterMode::Anisotropic:  return "Anisotropic"sv;
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown"sv;
	}

	std::string_view ToStringView(WrapMode wrapMode)
	{
		switch (wrapMode)
		{
			case WrapMode::None: return "None"sv;
			case WrapMode::Repeat: return "Repeat"sv;
			case WrapMode::Clamp:  return "Clamp"sv;
			case WrapMode::Mirror: return "Mirror"sv;
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown"sv;
	}

	FilterMode StringToFilterMode(std::string_view filterMode)
	{
		if (filterMode == "None") return FilterMode::None;
		if (filterMode == "Nearest") return FilterMode::Nearest;
		if (filterMode == "Linear") return FilterMode::Linear;
		if (filterMode == "Anisotropic") return FilterMode::Anisotropic;

		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return FilterMode::None;
	}

	WrapMode StringToWrapMode(std::string_view wrapMode)
	{
		if (wrapMode == "None") return WrapMode::None;
		if (wrapMode == "Repeat") return WrapMode::Repeat;
		if (wrapMode == "Clamp") return WrapMode::Clamp;
		if (wrapMode == "Mirror") return WrapMode::Mirror;

		SK_CORE_ASSERT(false, "Unkown wrap mode");
		return WrapMode::None;
	}

	Ref<Texture2D> Texture2D::Create()
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(specification, imageData);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Ref<TextureSource> textureSource)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(specification, textureSource);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(Ref<TextureSource> textureSource)
	{
		return Create({}, textureSource);
	}

	Ref<Texture2D> Texture2D::LoadFromDisc(const std::filesystem::path& filepath, const TextureSpecification& specification)
	{
		auto source = TextureImporter::ToTextureSourceFromFile(filepath);
		return Create(specification, source);
	}

	Ref<TextureCube> TextureCube::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTextureCube>::Create(specification, imageData);
		}

		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

}