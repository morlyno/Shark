
namespace Shark
{
	public static class ResourceManager
	{
		// File Path is relative to Assets
		public static AssetHandle GetAssetHandleFromFilePath(string filePath)
		{
			if (InternalCalls.ResourceManager_GetAssetHandleFromFilePath(filePath, out AssetHandle assetHandle))
				return assetHandle;
			return AssetHandle.Invalid;
		}

	}

}
