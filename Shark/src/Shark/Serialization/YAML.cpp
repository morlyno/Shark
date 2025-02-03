#include "skpch.h"
#include "YAML.h"

namespace YAML {

	Node LoadFile(const std::filesystem::path& filename) {
		std::ifstream fin(filename);
		if (!fin) {
			throw BadFile(filename.string());
		}
		return Load(fin);
	}

	Node LoadFile(const char* filename) {
		std::ifstream fin(filename);
		if (!fin) {
			throw BadFile(filename);
		}
		return Load(fin);
	}

}
