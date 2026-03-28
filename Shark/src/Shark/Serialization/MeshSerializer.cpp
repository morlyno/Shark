#include "skpch.h"
#include "MeshSerializer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Asset/AssetManager/AssetUtilities.h"
#include "Shark/Render/Mesh.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool MeshSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Mesh to {}", metadata.FilePath);

		ScopedTimer timer("Serializing Mesh");

		std::string result = SerializeToYAML(asset.As<Mesh>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!");
			return false;
		}

		const bool success = FileSystem::WriteString(GetAssetFilesystemPath(metadata), result);
		return success;
	}

	bool MeshSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Mesh from {}", metadata.FilePath);

		ScopedTimer timer(fmt::format("Loading Mesh [{}]", metadata.FilePath));

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		std::string filedata = FileSystem::ReadString(filesystemPath);
		if (filedata.empty())
		{
			context->OnFileEmpty(metadata);
			return false;
		}

		Ref<Mesh> mesh = nullptr;
		if (!DeserializeFromYAML(mesh, filedata, context))
		{
			context->OnYamlError(metadata);
			return false;
		}

		asset = mesh;
		asset->Handle = metadata.Handle;
		//context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	std::string MeshSerializer::SerializeToYAML(Ref<Mesh> mesh)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << mesh->m_MeshSource;
		out << YAML::Key << "Submeshes" << YAML::Value << mesh->m_Submeshes;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool MeshSerializer::DeserializeFromYAML(Ref<Mesh>& mesh, const std::string& filedata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node in = YAML::Load(filedata);
		if (!in)
			return false;

		auto meshNode = in["Mesh"];
		if (!meshNode)
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'Mesh' missing");
			return false;
		}

		mesh = Ref<Mesh>::Create();
		SK_DESERIALIZE_PROPERTY(meshNode, "MeshSource", mesh->m_MeshSource);
		SK_DESERIALIZE_PROPERTY(meshNode, "Submeshes", mesh->m_Submeshes);

		context->AddTask([mesh = mesh](AssetLoadContext* context)
		{
			auto future = AssetManager::GetAssetFuture(mesh->GetMeshSource());
			future.OnReady([context, mesh](Ref<Asset> asset)
			{
				mesh->InitializeFromThis(asset.As<MeshSource>());
				context->SetStatus(AssetLoadStatus::Ready);
			});
		});

		return true;
	}

}
