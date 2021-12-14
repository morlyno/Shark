#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Asset/AssetSerializer.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	class SceneSerializer : public SerializerBase
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath) override;

		virtual bool Serialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;

		bool Serialize(Ref<Scene> scene, const std::filesystem::path& filepath);
		bool Deserialize(Ref<Scene> scene, const std::filesystem::path& filepath);
	};

}
