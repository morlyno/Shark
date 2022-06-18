#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Asset/AssetSerializer.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	class SceneSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;
	};

}
