#pragma once

#include "Shark/Event/Event.h"

#include "Shark/File/FileWatcher.h"

namespace Shark {

	class ApplicationCloseEvent : public EventBase<EventTypes::ApplicationClosed, EventCategoryApplication>
	{
	public:
		ApplicationCloseEvent() = default;
	};

	class FileChangedEvent : public EventBase<EventTypes::FileChanged, EventCategoryApplication>
	{
	public:
		FileChangedEvent() = default;

		FileEvent GetFileEvent() const { return m_FileEvent; }
		const std::filesystem::path& GetFilePath() const { return m_FilePath; }
		const std::filesystem::path& GetOldFilePath() const { return m_OldFilePath; }

		virtual std::string ToString() const override { return fmt::format("{}, Event: {}, FilePath: {}, OldFilePath: {}", GetName(), FileEventToString(m_FileEvent), m_FilePath.string(), m_OldFilePath.string()); }
		
	private:
		FileEvent m_FileEvent;
		std::filesystem::path m_FilePath;
		std::filesystem::path m_OldFilePath;

		friend class FileWatcher;
	};

}