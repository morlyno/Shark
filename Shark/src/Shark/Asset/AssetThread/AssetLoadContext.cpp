#include "skpch.h"
#include "AssetLoadContext.h"

#include "Shark/Core/Project.h"
#include "Shark/File/FileSystem.h"

#include <future>

namespace Shark {

	AssetLoadContext::AssetLoadContext(AssetHandle handle)
		: m_Asset(handle)
	{
	}

	void AssetLoadContext::SetStatus(AssetLoadStatus status)
	{
		if (m_Status == AssetLoadStatus::Error)
			status = AssetLoadStatus::Error;

		if (status == AssetLoadStatus::Ready && !m_Tasks.empty())
			status = AssetLoadStatus::Loading;

		m_Status = status;
	}

	void AssetLoadContext::QueueStatus(AssetLoadStatus status)
	{
		AddTask([status](AssetLoadContext* context) { context->SetStatus(status); });
	}

	void AssetLoadContext::AddError(AssetLoadError error, std::string message)
	{
		m_Errors.emplace_back(error, std::move(message));
		SetStatus(AssetLoadStatus::Error);
	}

	AssetHandle AssetLoadContext::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		SK_CORE_VERIFY(asset->Handle == AssetHandle::Invalid);
		SK_CORE_VERIFY(!asset->IsFlagSet(AssetFlag::AsyncPending));
		asset->SetFlag(AssetFlag::AsyncPending);

		asset->Handle = AssetHandle::Generate();
		m_PendingAssets[asset->Handle] = asset;

		return asset->Handle;
	}

	std::filesystem::path AssetLoadContext::GetFilesystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::Absolute(metadata.FilePath);

		// #Investigate #async is accessing the project this way save?
		return (Project::GetActiveAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

	void AssetLoadContext::OnFileNotFound(const AssetMetaData& metadata)
	{
		AddError(
			AssetLoadError::FileNotFound,
			fmt::format("File '{}' not found", metadata.FilePath)
		);
	}

	void AssetLoadContext::OnFileEmpty(const AssetMetaData& metadata)
	{
		AddError(
			AssetLoadError::FileEmpty,
			fmt::format("File '{}' was empty", metadata.FilePath)
		);
	}

	void AssetLoadContext::OnYamlError(const AssetMetaData& metadata)
	{
		AddError(
			AssetLoadError::InvalidYAML,
			"Failed to deserialize YAML file"
		);
	}

	void AssetLoadContext::FixStatus(bool wasSuccessful)
	{
		if (!wasSuccessful || HasErrors())
		{
			SetStatus(AssetLoadStatus::Error);
			return;
		}

		SetStatus(AssetLoadStatus::Ready);
	}

}
