#pragma once

#include "Shark/Render/MeshSource.h";

#include <filesystem>

struct aiScene;

namespace Shark {

	class AssimpImporter
	{
	public:
		void Import(const std::filesystem::path& filePath);

	private:
#if 0
		std::vector<std::pair<std::string, std::vector<std::filesystem::path>>> LoadMaterials(const aiScene* scene, const std::filesystem::path& rootPath);
		void CreateMeshAssetFile(const std::filesystem::path& filepath, AssetHandle sourceHandle, const std::vector<AssetHandle>& materialHandles);
		void CreateMaterialAssetFile(const std::filesystem::path& filepath, const std::string& shaderName, const std::vector<AssetHandle>& textureHandles);
#else
		std::vector<std::filesystem::path> LoadTextures(const aiScene* scene, const std::filesystem::path& rootPath);
		void CreateMeshAssetFile(const std::filesystem::path& filepath, AssetHandle sourceHandle);
#endif

	private:
		std::filesystem::path m_SourcePath;
	};

}
