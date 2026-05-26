#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class AudioFile : public Asset
	{
	public:
		uint64_t Lenght     = 0;
		uint32_t SampleRate = 0;
		uint16_t Channels   = 0;
		uint16_t BitDepth   = 0;
		uint64_t FileSize   = 0;

		double LenghtInSenconds() const { return static_cast<double>(Lenght) / SampleRate; }

	public:
		static AssetType GetStaticType() { return AssetType::AudioFile; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	};

}
