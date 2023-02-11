#include "skpch.h"
#include "Font.h"

#if 1
#undef INFINITE
#include <msdf-atlas-gen.h>

#include "Platform/DirectX11/DirectXImage.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Renderer.h"

namespace Shark {

	struct FontData
	{
		msdfgen::FreetypeHandle* FreeType = nullptr;
	};
	static FontData* s_FontData = nullptr;

	template <typename T, typename S, int N, ImageFormat F, msdf_atlas::GeneratorFunction<S, N> GEN_FN>
	static Ref<Texture2D> CreateTextureAltas(const std::vector<msdf_atlas::GlyphGeometry>& glyphs, uint32_t width, uint32_t height)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GEN_FN, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(std::thread::hardware_concurrency());

		Timer timer;
		generator.generate(glyphs.data(), (int)glyphs.size());
		SK_CORE_TRACE_TAG("Font", "Generated Atlas in {}", timer.Elapsed());

		auto bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		msdfgen::savePng(bitmap, "FontAtlas.png");

		TextureSpecification spec;
		spec.Width = bitmap.width;
		spec.Height = bitmap.height;
		spec.Format = F;
		spec.MipLevels = 1;

		return Texture2D::Create(spec, { bitmap.pixels, (uint64_t)(bitmap.width * bitmap.height * N * sizeof(T)) });
	}

	void Font::Initialize()
	{
		SK_CORE_VERIFY(s_FontData == nullptr);
		s_FontData = new FontData();
		s_FontData->FreeType = msdfgen::initializeFreetype();
		SK_CORE_TRACE_TAG("Font", "Freetype Initialized");
		SK_CORE_VERIFY(s_FontData->FreeType, "Failed to initialize Freetype");
	}

	void Font::Shutdown()
	{
		msdfgen::deinitializeFreetype(s_FontData->FreeType);
		SK_CORE_TRACE_TAG("Font", "Freetype Deinitialized");
		delete s_FontData;
	}

	std::pair<Ref<Texture2D>, Ref<Texture2D>> Font::Test(const std::filesystem::path& fontPath)
	{
		std::string fontPathString = fontPath.string();

		msdfgen::FontHandle* font = msdfgen::loadFont(s_FontData->FreeType, fontPathString.c_str());
		if (!font)
		{
			SK_CORE_ERROR_TAG("Font", "Failed to load Font: {}", fontPathString);
			return {};
		}

		SK_CORE_TRACE_TAG("Font", "Font Loaded: {}", fontPathString);

		static const uint32_t charsetRange[] =
		{
			0x0020, 0x00FF
		};

		msdf_atlas::Charset charset = msdf_atlas::Charset::ASCII;
#if 0
		for (uint32_t i = 0; i < std::size(charsetRange); i += 2)
		{
			const uint32_t begin = charsetRange[i + 0];
			const uint32_t end   = charsetRange[i + 1];

			for (uint32_t c = begin; c <= end; c++)
				charset.add(c);
		}
#endif

		std::vector<msdf_atlas::GlyphGeometry> glyphs;
		auto fontGeometry = msdf_atlas::FontGeometry(&glyphs);
		int glyphsLoaded = fontGeometry.loadCharset(font, 1.0, charset);
		SK_CORE_TRACE_TAG("Font", "Loaded {} glyphs out of {}", glyphsLoaded, charset.size());

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(40);
		int result = atlasPacker.pack(glyphs.data(), glyphs.size());
		SK_CORE_ASSERT(result == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		int scale = atlasPacker.getScale();


#if 1
		// Edge Coloring
		for (msdf_atlas::GlyphGeometry& glyph : glyphs)
			glyph.edgeColoring(msdfgen::edgeColoringByDistance, 3.0, 0);
#endif

		Ref<Texture2D> texture = CreateTextureAltas<uint8_t, float, 4, ImageFormat::RGBA8, msdf_atlas::mtsdfGenerator>(glyphs, width, height);
		Ref<DirectXImage2D> hImage;

		std::wstring_view testString = L"Hallo";

		auto* glyph = fontGeometry.getGlyph(testString[0]);
		if (glyph)
		{
			int x, y, w, h;
			glyph->getBoxRect(x, y, w, h);
			SK_CORE_TRACE("l:{} b:{} r:{} t:{}", x, y, w, h);

			hImage = Ref<DirectXImage2D>::Create(ImageFormat::RGBA8, w, h, Buffer{});
			Renderer::Submit([texture, x, y, w, h, hImage]()
			{
				Ref<DirectXImage2D> image = texture->GetImage().As<DirectXImage2D>();
				auto* context = DirectXRenderer::GetContext();

				D3D11_BOX box{};
				box.left = x;
				box.top = y;
				box.right = x + w;
				box.bottom = y + h;
				box.back = 1;
				context->CopySubresourceRegion(hImage->GetResourceNative(), 0, 0, 0, 0, image->GetResourceNative(), 0, &box);
			});

		}

		msdfgen::destroyFont(font);

		return { texture, Texture2D::Create(hImage) };
	}

}
#endif
