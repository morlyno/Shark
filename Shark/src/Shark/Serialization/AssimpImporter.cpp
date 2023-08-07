#include "skpch.h"
#include "AssimpImporter.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Utils/YAMLUtils.h"

#include <yaml-cpp/yaml.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <numeric>

namespace Shark {

	void AssimpImporter::Import(const std::filesystem::path& filePath)
	{
		if (!FileSystem::Exists(filePath))
			return;

		const auto fsPath = FileSystem::GetAbsolute(filePath);
		if (ResourceManager::IsFileImported(fsPath))
			return;

		const std::string sourceName = filePath.stem().string();

		Assimp::Importer importer;
		const auto fsFilePath = FileSystem::GetAbsolute(filePath).string();
		const aiScene* scene = importer.ReadFile(fsFilePath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);

		if (!scene)
			return;

		AssetHandle meshSourceHandle = ResourceManager::ImportAsset(filePath);
		const auto textures = LoadTextures(scene, fsPath.parent_path());
		for (const auto& texturePath : textures)
		{
			if (!ResourceManager::IsFileImported(texturePath))
				ResourceManager::ImportAsset(texturePath);
		}

		const std::filesystem::path meshAssetPath = fmt::format("{}/Meshes/{}.skmesh", Project::GetAssetsPath().string(), filePath.stem().string());
		CreateMeshAssetFile(meshAssetPath, meshSourceHandle);
	}

	std::vector<std::filesystem::path> AssimpImporter::LoadTextures(const aiScene* scene, const std::filesystem::path& rootPath)
	{
		std::vector<std::filesystem::path> textures;

		for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
		{
			aiMaterial* material = scene->mMaterials[materialIndex];

			aiString textureFileName;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == aiReturn_SUCCESS)
			{
				auto texturePath = rootPath / textureFileName.C_Str();
				if (FileSystem::Exists(texturePath))
					textures.emplace_back(texturePath);
			}

		}

		return textures;
	}

	void AssimpImporter::CreateMeshAssetFile(const std::filesystem::path& filepath, AssetHandle sourceHandle)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << sourceHandle;
		out << YAML::Key << "SubmeshIndices" << YAML::Value << std::vector<uint32_t>{};
		out << YAML::Key << "Materials" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::EndMap;
		out << YAML::EndMap;
		out << YAML::EndMap;
		FileSystem::WriteString(filepath, out.c_str());

		ResourceManager::ImportAsset(filepath);
	}

}
