#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"
	
namespace Shark {

	enum class AssetLoadStatus
	{
		Ready,
		Loading,
		Error
	};

	enum class AssetLoadError
	{
		None = 0,
		Unknown,
		FileNotFound,
		FileEmpty,
		InvalidYAML,
		Deprecated,
		TaskFailed
	};

	class AssetLoadContext
	{
	public:
		AssetLoadContext(AssetHandle handle);

		void SetStatus(AssetLoadStatus status);
		void QueueStatus(AssetLoadStatus status);
		void AddError(AssetLoadError error, std::string message);
		void AddTask(std::invocable auto&& func)
		{
			AddTask([func = std::move(func)](AssetLoadContext*) { func(); });
		}
		void AddTask(std::invocable<AssetLoadContext*> auto&& func)
		{
			m_Tasks.emplace_back(std::move(func));
		}

		AssetHandle AddMemoryOnlyAsset(Ref<Asset> asset);
		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	public: // Error utilities
		void OnFileNotFound(const AssetMetaData& metadata);
		void OnFileEmpty(const AssetMetaData& metadata);
		void OnYamlError(const AssetMetaData& metadata);

		auto GetStatus() const { return m_Status; }
		bool HasErrors() const { return m_Status == AssetLoadStatus::Error; }
		bool Loading() const { return m_Status == AssetLoadStatus::Loading; }
		const auto& GetErrors() const { return m_Errors; }

		AssetHandle GetAssetHandle() const { return m_Asset; }
		bool HasTasks() const { return !m_Tasks.empty(); }
		auto  GetTasks() { return std::move(m_Tasks); }
		auto& GetAssets() { return m_PendingAssets; }

		void FixStatus(bool wasSuccessful);

	public:
		struct Error
		{
			AssetLoadError ErrorCode;
			std::string Message;
		};

	private:
		AssetHandle m_Asset;
		AssetLoadStatus m_Status = AssetLoadStatus::Loading;

		std::vector<Error> m_Errors;
		std::vector<std::function<void(AssetLoadContext*)>> m_Tasks;
		std::unordered_map<AssetHandle, Ref<Asset>> m_PendingAssets;
	};

}

template<>
struct fmt::formatter<Shark::AssetLoadContext::Error>
{
	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const Shark::AssetLoadContext::Error& error, FormatContext& ctx) const -> FormatContext::iterator
	{
		return fmt::format_to(ctx.out(), "[{}] {}", error.ErrorCode, error.Message);
	}
};
