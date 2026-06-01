#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class SoundConfig : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::SoundConfig; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		AssetHandle AudioSourceHandle = AssetHandle::Invalid;
		bool IsLooping = false;
		float VolumeMultiplier = 1.0;
		float PitchMultiplier = 1.0f;

	};

}
