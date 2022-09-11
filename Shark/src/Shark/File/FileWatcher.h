#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace NotifyFilter {
		enum Type : uint16_t
		{
			None = 0,
			FileName = BIT(0),
			DirName = BIT(1),
			Size = BIT(3),
			LastWrite = BIT(4),
			LastAccess = BIT(5),
			Creation = BIT(6),

			All = FileName | DirName | Size | LastWrite | LastAccess | Creation,
			Default = FileName | Size | Creation
		};
		using Flags = std::underlying_type_t<Type>;
	}

	namespace EventFilter {
		enum Type : uint16_t
		{
			None = 0,
			Created = BIT(0),
			Deleted = BIT(1),
			Modified = BIT(2),
			Renamed = BIT(3),

			All = Created | Deleted | Modified | Renamed,
		};
		using Flags = std::underlying_type_t<Type>;
	}

	enum class FileEvent
	{
		None = 0,
		Created,
		Deleted,
		Modified,
		OldName,
		NewName
	};

	inline std::string ToString(FileEvent fileEvent)
	{
		switch (fileEvent)
		{
			case FileEvent::None: return "None";
			case FileEvent::Created: return "Created";
			case FileEvent::Deleted: return "Deleted";
			case FileEvent::Modified: return "Modified";
			case FileEvent::OldName: return "OldName";
			case FileEvent::NewName: return "NewName";
		}
		SK_CORE_ASSERT(false, "Unkown File Event");
		return "Unkown";
	}

	struct FileChangedData
	{
		FileEvent Type;
		std::filesystem::path FilePath;
		bool IsDirectory = false;
	};

	using FileWatcherCallbackFunc = std::function<void(const std::vector<FileChangedData>&)>;

	struct WatchingSettings
	{
		FileWatcherCallbackFunc Callback = nullptr;
		NotifyFilter::Flags NofityFilter = NotifyFilter::Default;
		EventFilter::Flags EnabledEvents = EventFilter::All;
		bool IsRecursive = true;
	};

	class FileWatcher : public RefCount
	{
	public:
		virtual ~FileWatcher() = default;

		virtual void StartWatching(const std::string& key, const std::filesystem::path& dirPath, FileWatcherCallbackFunc callback) = 0;
		virtual void StartWatching(const std::string& key, const std::filesystem::path& dirPath, const WatchingSettings& settings) = 0;
		virtual void StopWatching(const std::string& key) = 0;
		virtual void SetCallback(const std::string& key, FileWatcherCallbackFunc callback) = 0;

		virtual bool IsWatching(const std::string& key) = 0;

		virtual void Update() = 0;

	public:
		static Ref<FileWatcher> Create();

	};

}

template<>
struct fmt::formatter<Shark::FileChangedData>
{
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const Shark::FileChangedData& data, FormatContext& ctx) -> decltype(ctx.out())
	{
		format_to(ctx.out(), "({0}) {1}", Shark::ToString(data.Type), data.FilePath);
		return ctx.out();
	}

};
