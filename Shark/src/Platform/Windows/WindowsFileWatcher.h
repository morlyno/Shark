#pragma once

#include "Shark/Core/Base.h"
#include "Shark/File/FileWatcher.h"

#include <filesystem>

namespace Shark {

	class WindowsFileWatcher : public FileWatcher
	{
	public:
		WindowsFileWatcher() = default;
		WindowsFileWatcher(const FileWatcherSpecification& specification);
		~WindowsFileWatcher();

		virtual void Start() override;
		virtual void Stop() override;

		virtual bool IsRunning() override { return m_Running; }
		virtual bool IsPaused() override { return m_Paused; }

		virtual void SkipNextEvent() override { m_SkipNextEvent = true; }
		virtual void Pause() override { m_Paused = true; }
		virtual void Continue() override { m_Paused = false; }

		virtual void SetCallback(FileWatcherCallbackFunc callback) override { m_Specs.CallbackFunc = callback; }
		virtual void SetDirectory(const std::filesystem::path& directory, bool restartIfRunning) override;

		virtual const FileWatcherSpecification& GetSpecification() const override { return m_Specs; }
		virtual void SetSpecification(const FileWatcherSpecification& specs) override { m_Specs = specs; }

	private:
		void WatcherFunction();

	private:
		std::thread m_Thread;
		FileWatcherSpecification m_Specs;

		bool m_Running = false;
		bool m_SkipNextEvent = false;
		bool m_Paused = false;

		HANDLE m_StopThreadEvent = NULL;
	};

}
