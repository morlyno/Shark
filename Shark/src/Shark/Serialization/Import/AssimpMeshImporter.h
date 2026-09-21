#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {
	class MeshSource;
	class Skeleton;
	class Animation;
	class AssetLoadContext;

	class Image2D;
}

struct aiScene;
struct aiNode;
struct aiString;

namespace Shark {

	class AssimpMeshImporter
	{
	private:
		struct UploadJob
		{
			Ref<Image2D> Image;
			UniqueBuffer Data;

			UploadJob(RefArg<Image2D> Image, UniqueBuffer Data);
			UploadJob(UploadJob&&)            /* = default */;
			UploadJob& operator=(UploadJob&&) /* = default */;
			~UploadJob()                      /* = default */;
		};

	public:
		AssimpMeshImporter(const std::filesystem::path& filepath);

		std::vector<UploadJob>& GetJobs();
		Ref<MeshSource> ToMeshSourceFromFile(AssetLoadContext* context);
		Scope<Skeleton> ImportSkeleton(const aiScene* scene);
		Scope<Animation> ImportAnimation(const aiScene* scene, uint32_t animationIndex, const Skeleton& skeleton);

	private:
		AssetHandle LoadTexture(const aiScene* scene, const aiString& path, bool sRGB, AssetLoadContext* context);

		void TraverseNodes(Ref<MeshSource> meshSource, aiNode* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
		void TraverseNodes(aiNode* node, Skeleton* skeleton, std::set<std::string_view>& bones);
	private:
		std::filesystem::path m_Filepath;
		std::string m_Extension;

		std::vector<UploadJob> m_Jobs;
	};

}
