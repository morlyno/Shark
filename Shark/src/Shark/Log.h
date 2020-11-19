#pragma once

#include <memory>

#include "Core.h"

#include <spdlog/spdlog.h>

namespace Shark {

	class SHARK_API Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_Core_Logger; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_Client_Logger; }
	private:
		static std::shared_ptr<spdlog::logger> s_Core_Logger;
		static std::shared_ptr<spdlog::logger> s_Client_Logger;
	};

}
