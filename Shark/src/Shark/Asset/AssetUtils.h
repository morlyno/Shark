
#include "Shark/Core/Base.h"

namespace Shark {

	enum class AssetType;

	class AssetUtils
	{
	public:
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& assetPath);
	};

}
