#include "skpch.h"
#include "Font.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MSDFData.h"

#undef INFINITE
#include <msdf-atlas-gen.h>

namespace Shark {

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

	Font::Font(const std::filesystem::path& fontPath)
	{
		m_MSDFData = new MSDFData();
		Init(fontPath);
	}

	Font::~Font()
	{
		delete m_MSDFData;
	}

	void Font::Init(const std::filesystem::path& fontPath)
	{
		msdfgen::FreetypeHandle* freetype = msdfgen::initializeFreetype();

		std::string fontPathString = fontPath.string();

		msdfgen::FontHandle* font = msdfgen::loadFont(freetype, fontPathString.c_str());
		if (!font)
		{
			SK_CORE_ERROR_TAG("Font", "Failed to load Font: {}", fontPathString);
			return;
		}

		SK_CORE_TRACE_TAG("Font", "Font Loaded: {}", fontPathString);

		static const uint32_t charsetRange[] =
		{
			0x0020, 0x00FF
		};

		msdf_atlas::Charset charset;
		for (uint32_t i = 0; i < std::size(charsetRange); i += 2)
		{
			const uint32_t begin = charsetRange[i + 0];
			const uint32_t end   = charsetRange[i + 1];

			for (uint32_t c = begin; c <= end; c++)
				charset.add(c);
		}

		m_MSDFData->FontGeometry = msdf_atlas::FontGeometry(&m_MSDFData->Glyphs);
		int glyphsLoaded = m_MSDFData->FontGeometry.loadCharset(font, 1.0, charset);
		SK_CORE_TRACE_TAG("Font", "Loaded {} glyphs out of {}", glyphsLoaded, charset.size());

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(40);
		int result = atlasPacker.pack(m_MSDFData->Glyphs.data(), m_MSDFData->Glyphs.size());
		SK_CORE_ASSERT(result == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		int scale = atlasPacker.getScale();


		// Edge Coloring
		for (msdf_atlas::GlyphGeometry& glyph : m_MSDFData->Glyphs)
			glyph.edgeColoring(msdfgen::edgeColoringByDistance, 3.0, 0);

		m_FontAtlas = CreateTextureAltas<uint8_t, float, 4, ImageFormat::RGBA8, msdf_atlas::mtsdfGenerator>(m_MSDFData->Glyphs, width, height);

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(freetype);
	}

}