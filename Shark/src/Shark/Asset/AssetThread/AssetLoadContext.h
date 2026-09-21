#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"
	
namespace Shark {

	enum class AssetLoadStatus
	{
		Auto,
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
		using Task = std::move_only_function<bool(AssetLoadContext*)>;

	public:
		AssetLoadContext(AssetHandle handle);

		void SetStatus(AssetLoadStatus status);
		void AddError(AssetLoadError error, std::string message);
		void AddTask(Task task);

		AssetHandle AddMemoryOnlyAsset(Ref<Asset> asset);
		std::filesystem::path GetFilesystemPath(const AssetMetaData& metadata);

	public: // Error utilities
		void OnFileNotFound(const AssetMetaData& metadata);
		void OnFileEmpty(const AssetMetaData& metadata);
		void OnYamlError(const AssetMetaData& metadata);

		bool HasErrors() const { return m_Status == AssetLoadStatus::Error; }
		bool Loading() const;
		const auto& GetErrors() const { return m_Errors; }

		AssetHandle GetAssetHandle() const { return m_Asset; }
		bool HasTasks() const { return !m_Tasks.empty(); }
		auto& GetTasks() { return m_Tasks; }
		auto& GetAssets() { return m_PendingAssets; }

		void SetErrorFallback(Ref<Asset> fallback) { m_ErrorFallback = fallback; }
		auto GetErrorFallback() const { return m_ErrorFallback; }

		void FixStatus(bool wasSuccessful);

	public:
		struct Error
		{
			AssetLoadError ErrorCode;
			std::string Message;
		};

	private:
		AssetHandle m_Asset;
		AssetLoadStatus m_Status = AssetLoadStatus::Auto;

		std::vector<Error> m_Errors;
		Ref<Asset> m_ErrorFallback;

		std::vector<Task> m_Tasks;
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
