#pragma once

#include "Shark/Core/Base.h"

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include "spdlog/sinks/base_sink.h"

namespace Shark {
	
	struct ConsoleSinkMessage
	{
		LogLevel MessageLevel;
		std::string Message;
		std::chrono::system_clock::time_point Time;

		ConsoleSinkMessage(LogLevel level, std::string&& msg, std::chrono::system_clock::time_point time)
			: MessageLevel(level), Message(std::move(msg)), Time(time) {}
	};

	class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		ConsoleSink(uint32_t capacity = 10);
		~ConsoleSink();

		void SetPushMessageCallback(std::function<void(ConsoleSinkMessage&&)> callback) { m_PushMessageCallback = callback; }

	protected:
		virtual void sink_it_(const spdlog::details::log_msg& msg) override;
		virtual void flush_() override;

	private:
		LogLevel GetMessageLevel(spdlog::level::level_enum level) const;

	private:
		uint32_t m_MessageBufferCapacity;
		std::vector<ConsoleSinkMessage> m_MessageBuffer;

		std::function<void(ConsoleSinkMessage&&)> m_PushMessageCallback = nullptr;
	};

}
