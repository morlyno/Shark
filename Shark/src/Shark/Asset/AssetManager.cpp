#include "skpch.h"
#include "AssetManager.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetManager/AssetManagerBase.h"

namespace Shark {

	AssetType AssetManager::GetAssetType(AssetHandle handle)
	{
		return Project::GetAssetManager()->GetAssetType(handle);
	}

	Threading::Future<Ref<Asset>> AssetManager::GetAssetFuture(AssetHandle handle)
	{
		return Project::GetAssetManager()->GetAssetFuture(handle);
	}

	bool AssetManager::WaitForAsset(AssetHandle handle, bool queueLoad)
	{
		return Project::GetAssetManager()->WaitForAsset(handle, queueLoad);
	}

	void AssetManager::LoadAssetAsync(AssetHandle handle)
	{
		return Project::GetAssetManager()->LoadAssetAsync(handle);
	}

	std::vector<AssetHandle> AssetManager::GetAllAssetsOfType(AssetType assetType)
	{
		return Project::GetAssetManager()->GetAllAssetsOfType(assetType);
	}

	AssetHandle AssetManager::AddMemoryAsset(Ref<Asset> asset)
	{
		return Project::GetAssetManager()->AddMemoryAsset(asset);
	}

	bool AssetManager::ReloadAsset(AssetHandle handle)
	{
		return Project::GetAssetManager()->ReloadAsset(handle);
	}

	void AssetManager::ReloadAssetAsync(AssetHandle handle)
	{
		Project::GetAssetManager()->ReloadAssetAsync(handle);
	}

	bool AssetManager::DependenciesLoaded(AssetHandle handle, bool loadIfNotReady)
	{
		return Project::GetAssetManager()->DependenciesLoaded(handle, loadIfNotReady);
	}

	bool AssetManager::IsValidAssetHandle(AssetHandle handle)
	{
		return Project::GetAssetManager()->IsValidAssetHandle(handle);
	}

	bool AssetManager::IsMemoryAsset(AssetHandle handle)
	{
		return Project::GetAssetManager()->IsMemoryAsset(handle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return Project::GetAssetManager()->IsAssetLoaded(handle);
	}

	void AssetManager::DeleteAsset(AssetHandle handle)
	{
		return Project::GetAssetManager()->DeleteAsset(handle);
	}

	void AssetManager::DeleteMemoryAsset(AssetHandle handle)
	{
		return Project::GetAssetManager()->DeleteMemoryAsset(handle);
	}

	void AssetManager::SyncWithAssetThread()
	{
		Project::GetAssetManager()->SyncWithAssetThread();
	}

	Ref<Asset> AssetManager::GetAsset(AssetHandle handle)
	{
		return Project::GetAssetManager()->GetAsset(handle);
	}

	Ref<Asset> AssetManager::GetAssetAsync(AssetHandle handle)
	{
		return Project::GetAssetManager()->GetAssetAsync(handle);
	}

}
