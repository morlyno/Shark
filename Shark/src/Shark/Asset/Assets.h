#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	class TextureSource : public Asset
	{
	public:
		TextureSource() = default;
		virtual ~TextureSource() = default;

		static AssetType GetStaticType() { return AssetType::TextureSource; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<TextureSource> Create() { return Ref<TextureSource>::Create(); }
	};

	class ScriptFile : public Asset
	{
	public:
		ScriptFile() = default;
		~ScriptFile() = default;

		static AssetType GetStateType() { return AssetType::ScriptFile; }
		virtual AssetType GetAssetType() const override{ return GetStateType(); }

		static Ref<ScriptFile> Create() { return Ref<ScriptFile>::Create(); }
	};

}
