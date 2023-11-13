#pragma once

#pragma push_macro("INFINITE")
#undef INFINITE
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#pragma pop_macro("INFINITE")

#include <vector>

namespace Shark {

	struct MSDFData
	{
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
		msdf_atlas::FontGeometry FontGeometry;
	};

}
