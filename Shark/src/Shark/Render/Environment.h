#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {
	class TextureCube;
}

namespace Shark {

	class Environment : public Asset
	{
	public:
		Environment(Ref<TextureCube> radianceMap, Ref<TextureCube> irradianceMap);

		RefArg<TextureCube> GetRadianceMap() const { return m_RadianceMap; }
		RefArg<TextureCube> GetIrradianceMap() const { return m_IrradianceMap; }

	public: // Asset Interface
		static AssetType GetStaticType() { return AssetType::Environment; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	private:
		Ref<TextureCube> m_RadianceMap;
		Ref<TextureCube> m_IrradianceMap;

		friend class EnvironmentSerializer;
	};

}
