#pragma once

#include "Shark/Core/Base.h"
#include "Shark/File/FileWatcher.h"

#include <filesystem>

namespace Shark {

	struct WatchData;

	class WindowsFileWatcher : public FileWatcher
	{
	public:
		virtual ~WindowsFileWatcher();

		virtual void StartWatching(const std::string& key, const std::filesystem::path& dirPath, FileWatcherCallbackFunc callback) override;
		virtual void StartWatching(const std::string& key, const std::filesystem::path& dirPath, const WatchingSettings& settings) override;
		virtual void StopWatching(const std::string& key) override;
		virtual void SetCallback(const std::string& key, FileWatcherCallbackFunc callback) override;

		virtual bool IsWatching(const std::string& key) override;

		virtual void Update() override;

		virtual uint32_t GetActiveCount() const override { return (uint32_t)m_Watches.size(); }

	private:
		std::map<std::string, WatchData*> m_Watches;
	};

}
