#pragma once

#include "Shark/Asset/AssetThread/AssetLoadContext.h"
#include "Shark/Render/MeshSource.h"

struct aiScene;
struct aiNode;
struct aiString;

namespace Shark {

	class AssimpMeshImporter
	{
	public:
		AssimpMeshImporter(const std::filesystem::path& filepath);

		Ref<MeshSource> ToMeshSourceFromFile(AssetLoadContext* context);
	private:
		void TraverseNodes(Ref<MeshSource> meshSource, aiNode* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		AssetHandle LoadTexture(const aiScene* scene, const aiString& path, bool sRGB, AssetLoadContext* context);
	private:
		std::filesystem::path m_Filepath;
		std::string m_Extension;
	};

}
