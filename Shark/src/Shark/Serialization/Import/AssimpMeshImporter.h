#pragma once

#include "Shark/Render/MeshSource.h"

struct aiScene;
struct aiNode;
struct aiString;

namespace Shark {

	class AssimpMeshImporter
	{
	public:
		AssimpMeshImporter(const std::filesystem::path& filepath);

		Ref<MeshSource> ToMeshSourceFromFile();
	private:
		void TraverseNodes(Ref<MeshSource> meshSource, aiNode* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = {}, uint32_t level = 0);

		AssetHandle LoadTexture(const aiScene* scene, const aiString& path);
	private:
		std::filesystem::path m_Filepath;
		std::string m_Extension;
	};

}