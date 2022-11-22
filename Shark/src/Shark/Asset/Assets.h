#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

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
