#pragma once

#include "Shark/Render/Mesh.h"

#include <filesystem>

struct aiNode;

namespace Shark {

	class AssimpImporter
	{
	public:
		Ref<Mesh> TryLoad(const std::filesystem::path& filePath);

	private:
		void AddNode(Mesh::Node* meshNode, aiNode* node);

	};

}
