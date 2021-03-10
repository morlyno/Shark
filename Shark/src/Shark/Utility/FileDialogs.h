#pragma once

#include <optional>

namespace Shark {

	namespace FileDialogs {

		std::optional<std::string> OpenFile(const char* filter);
		std::optional<std::string> SaveFile(const char* filter);

	};

}
