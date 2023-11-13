#include "skpch.h"
#include "RuntimeAssetManager.h"

namespace Shark {

	RuntimeAssetManager::RuntimeAssetManager()
	{
	}

	RuntimeAssetManager::~RuntimeAssetManager()
	{
	}

	Ref<Asset> RuntimeAssetManager::GetAsset(AssetHandle handle)
	{
		return nullptr;
	}

	AssetHandle RuntimeAssetManager::AddMemoryAsset(Ref<Asset> asset)
	{
		return AssetHandle::Invalid;
	}

	AssetType RuntimeAssetManager::GetAssetType(AssetHandle handle) const
	{
		return AssetType::None;
	}

	bool RuntimeAssetManager::IsMemoryAsset(AssetHandle handle) const
	{
		return false;
	}

	bool RuntimeAssetManager::IsValidAssetHandle(AssetHandle handle) const
	{
		return false;
	}

	bool RuntimeAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return false;
	}

}
