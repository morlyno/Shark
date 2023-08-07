#pragma once

#include "Shark/Serialization/SerializerBase.h"

#include "Shark/Render/MeshSource.h"

struct aiScene;
struct aiNode;

namespace Shark {

	struct MeshSourceSerializerSettings
	{
		bool GenerateMips = true;
		bool Anisotropy = false;
		uint32_t MaxAnisotropy = 16;
	};
	extern MeshSourceSerializerSettings g_MeshSourceSerializerSettings;

	class MeshSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) override;
		
	private:
		bool TryLoad(Ref<MeshSource> meshSource, const std::filesystem::path& filepath);
		void LoadBuffersAndSubMeshes(const aiScene* scene, Ref<VertexBuffer>& outVertexBuffer, Ref<IndexBuffer>& outIndexBuffer, std::vector<MeshSource::SubMesh>& outSubMeshes);
		Ref<MaterialTable> LoadMaterialTable(const aiScene* scene, const std::filesystem::path& rootPath);
		void UpdateMaterials(Ref<MaterialTable> materialTable);
		void AddNode(MeshSource::Node* meshNode, aiNode* node);
	};

}
