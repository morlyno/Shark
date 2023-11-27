#include "skpch.h"
#include "TextureSerializers.h"

#include "Shark/Render/Image.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <stb_image.h>
#include <yaml-cpp/yaml.h>

#define SK_SERIALIZATION_ERROR(...) SK_CORE_ERROR_TAG("Serialization", __VA_ARGS__); SK_DEBUG_BREAK();

namespace Shark {

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Texture Source Serializer ///////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool TextureSourceSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool TextureSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading TextureSource from {}", metadata.FilePath);
		ScopedTimer timer("Loading TextureSource");

		const auto filesystemPath = Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		auto source = Ref<TextureSource>::Create();
		source->ImageData = TextureImporter::ToBufferFromFile(filesystemPath, source->Format, source->Width, source->Height);
		if (!source->ImageData)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Import Texture");
			return false;
		}

		asset = source;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool TextureSourceSerializer::TryLoadAssetFromTexture(Ref<TextureSource>& textureSource, const std::filesystem::path& filepath)
	{
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Texture Serializer //////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool TextureSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Texture to {}", metadata.FilePath);
		Timer timer;

		//if (!ResourceManager::HasExistingFilePath(metadata))
		//{
		//	SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
		//	return false;
		//}

		std::string result = SerializeToYAML(asset.As<Texture2D>());
		if (result.empty())
		{
			SK_SERIALIZATION_ERROR("YAML result was null!");
			return false;
		}

		std::ofstream fout(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		SK_CORE_ASSERT(fout);

		fout << result;
		fout.close();

		SK_CORE_INFO_TAG("Serialization", "Serializing Texture took {}", timer.Elapsed());
		return true;
	}

	bool TextureSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG("Serialization", "Loading Texture from {}", metadata.FilePath);
		Timer timer;

		if (!Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_SERIALIZATION_ERROR("Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		if (filedata.empty())
		{
			SK_SERIALIZATION_ERROR("File was empty!");
			return false;
		}

		Ref<Texture2D> texture = Texture2D::Create();
		if (!DesrializeFromYAML(texture, filedata))
		{
			SK_SERIALIZATION_ERROR("Failed to load data from YAML!");
			return false;
		}

		asset = texture;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Texture took {}", timer.Elapsed());
		return true;
	}

	std::string TextureSerializer::SerializeToYAML(Ref<Texture2D> texture)
	{
		SK_PROFILE_FUNCTION();

		const TextureSpecification& specification = texture->GetSpecification();
		AssetHandle sourceHandle = texture->GetTextureSource()->Handle;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Texture";

		out << YAML::BeginMap;
		out << YAML::Key << "TextureSource" << YAML::Value << sourceHandle;
		out << YAML::Key << "Format" << YAML::Value << ToString(specification.Format);
		out << YAML::Key << "GenerateMips" << YAML::Value << specification.GenerateMips;
		out << YAML::Key << "Sampler";

		out << YAML::BeginMap;
		out << YAML::Key << "Filter" << YAML::Value << ToString(specification.Sampler.Filter);
		out << YAML::Key << "Wrap" << YAML::Value << ToString(specification.Sampler.Wrap);
		out << YAML::Key << "Anisotropy" << YAML::Value << specification.Sampler.Anisotropy;
		out << YAML::Key << "MaxAnisotropy" << YAML::Value << specification.Sampler.MaxAnisotropy;
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool TextureSerializer::DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node node = YAML::Load(filedata);

		YAML::Node textureNode = node["Texture"];
		if (!textureNode)
			return false;

		AssetHandle sourceHandle = textureNode["TextureSource"].as<AssetHandle>();
		Ref<TextureSource> textureSource = AssetManager::GetAsset<TextureSource>(sourceHandle);

		auto& specification = texture->GetSpecificationMutable();
		specification.GenerateMips = textureNode["GenerateMips"].as<bool>(true);

		if (auto samplerNode = textureNode["Sampler"])
		{
			auto& sampler = specification.Sampler;
			sampler.Filter = StringToFilterMode(samplerNode["Filter"].as<std::string>("Linear"));
			sampler.Wrap = StringToWrapMode(samplerNode["Wrap"].as<std::string>("Repeat"));
			sampler.Anisotropy = samplerNode["Anisotropy"].as<bool>();
			sampler.MaxAnisotropy = samplerNode["MaxAnisotropy"].as<uint32_t>();
		}

		SK_CORE_TRACE_TAG(Tag::Serialization, " - Generate Mips {}", specification.GenerateMips);
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Filter {}", ToStringView(specification.Sampler.Filter));
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Wrap {}", ToStringView(specification.Sampler.Wrap));
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Anisotropy Enabled {}", specification.Sampler.Anisotropy);
		SK_CORE_TRACE_TAG(Tag::Serialization, " - Max Anisotropy {}", specification.Sampler.MaxAnisotropy);

#if 0
		Application::Get().SubmitToMainThread([texture, textureSource]()
		{
			texture->SetTextureSource(textureSource);
			texture->Invalidate();
		});
#else
		texture->SetTextureSource(textureSource);
		texture->Invalidate();
#endif
		return true;
	}

}
