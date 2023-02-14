#pragma once

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <vector>

namespace Shark {

	struct MSDFData
	{
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
		msdf_atlas::FontGeometry FontGeometry;
	};

}
