#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace NotifyFlag {

		enum Type : uint32_t
		{
			None = 0,
			FileName = BIT(0),
			DirectoryName = BIT(1),
			Attributes = BIT(2),
			Size = BIT(3),
			LastWrite = BIT(4),
			LastAccess = BIT(5),
			Creation = BIT(6),
			Security = BIT(7),

			All = FileName | DirectoryName | Attributes | Size | LastWrite | LastAccess | Creation | Security
		};

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


	struct FileWatcherSpecification
	{
		std::filesystem::path Directory;
		NotifyFlag::Type NotifyFlags;
		bool WatchSubTrees;
		FileWatcherCallbackFunc CallbackFunc;
	};

	class FileWatcher : public RefCount
	{
	public:
		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual bool IsRunning() = 0;
		virtual bool IsPaused() = 0;

		virtual void SkipNextEvent() = 0;
		virtual void Pause() = 0;
		virtual void Continue() = 0;

		virtual void SetCallback(FileWatcherCallbackFunc callback) = 0;
		virtual void SetDirectory(const std::filesystem::path& directory, bool restartIfRunning) = 0;

		virtual const FileWatcherSpecification& GetSpecification() const = 0;
		virtual void SetSpecification(const FileWatcherSpecification& specs) = 0;

		static Ref<FileWatcher> Create(const FileWatcherSpecification& specs);
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