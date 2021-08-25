#include "skpch.h"
#include "YAMLUtils.h"

namespace YAML {

	Node LoadFile(const std::filesystem::path& filename) {
		std::ifstream fin(filename);
		if (!fin) {
			throw BadFile(filename.string());
		}
		return Load(fin);
	}

}
