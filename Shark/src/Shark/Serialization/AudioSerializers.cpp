#include "skpch.h"
#include "AudioSerializers.h"

#include "Shark/Core/Application.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Audio/AudioEngine.h"
#include "Shark/Audio/AudioFile.h"
#include "Shark/Audio/SoundConfig.h"

#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializerUtilities.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool AudioFileSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool AudioFileSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto audioFile = Application::Get().GetAudioEngine()->QueryFileInfo(metadata.Handle);
		if (!audioFile)
		{
			context->AddError(AssetLoadError::Unknown, "AudioEngine.QueryFileInfo returned null");
			return false;
		}

		asset = audioFile;
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	bool SoundConfigSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		std::string result = SerializeToYAML(asset.As<SoundConfig>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!");
			return false;
		}

		const auto filesystemPath = Utilities::GetAssetFilesystemPath(metadata);
		FileSystem::WriteString(filesystemPath, result);

		return true;
	}

	bool SoundConfigSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

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

		auto soundConfig = Ref<SoundConfig>::Create();
		if (!DeserializeFromYAML(soundConfig, filedata, context))
		{
			context->OnYamlError(metadata);
			return false;
		}

		asset = soundConfig;
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	std::string SoundConfigSerializer::SerializeToYAML(Ref<SoundConfig> soundConfig)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "SoundConfig" << YAML::Value;
		
		out << YAML::BeginMap;
		out << YAML::Key << "AudioSourceHandle" << YAML::Value << soundConfig->AudioSourceHandle;
		out << YAML::Key << "IsLooping" << YAML::Value << soundConfig->IsLooping;
		out << YAML::Key << "VolumeMultiplier" << YAML::Value << soundConfig->VolumeMultiplier;
		out << YAML::Key << "PitchMultiplier" << YAML::Value << soundConfig->PitchMultiplier;
		out << YAML::EndMap;

		out << YAML::EndMap;
		return out.c_str();
	}

	bool SoundConfigSerializer::DeserializeFromYAML(Ref<SoundConfig> soundConfig, const std::string& filedata, AssetLoadContext* context)
	{
		auto rootNode = YAML::Load(filedata);
		if (!rootNode)
			return false;

		auto soundConfigNode = rootNode["SoundConfig"];
		if (!soundConfigNode)
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'SoundConfig' missing");
			return false;
		}

		YAML::DeserializeProperty(soundConfigNode, "AudioSourceHandle", soundConfig->AudioSourceHandle);
		YAML::DeserializeProperty(soundConfigNode, "IsLooping", soundConfig->IsLooping);
		YAML::DeserializeProperty(soundConfigNode, "VolumeMultiplier", soundConfig->VolumeMultiplier);
		YAML::DeserializeProperty(soundConfigNode, "PitchMultiplier", soundConfig->PitchMultiplier);

		context->AddTask([soundConfig](AssetLoadContext* context)
		{
			if (AssetManager::GetAssetType(soundConfig->AudioSourceHandle) != AssetType::AudioFile)
				soundConfig->AudioSourceHandle = AssetHandle::Invalid;

			context->SetStatus(AssetLoadStatus::Ready);
		});

		return true;
	}

}
