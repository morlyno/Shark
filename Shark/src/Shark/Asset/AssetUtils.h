
#include "Shark/Core/Base.h"

namespace Shark {

	class Asset;
	enum class AssetType;

	class AssetUtils
	{
	public:
		static Ref<Asset> Create(AssetType assetType);

		static AssetType GetAssetTypeFromPath(const std::filesystem::path& assetPath);
	};

}
