#pragma once

#include <optional>

namespace Shark {

	namespace FileDialogs {

		std::string OpenFile(const char* filter);
		std::string SaveFile(const char* filter);

	};

	namespace Utility {

		void OpenExplorer(const std::string& path);

	}

}
