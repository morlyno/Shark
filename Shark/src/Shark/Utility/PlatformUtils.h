#pragma once

namespace Shark {

	namespace FileDialogs {

		SK_DEPRECATED("Use OpenFileW instead");
		std::string OpenFile(const char* filter);
		SK_DEPRECATED("Use SaveFileW instead");
		std::string SaveFile(const char* filter);

		std::filesystem::path OpenFileW(const wchar_t* filter);
		std::filesystem::path SaveFileW(const wchar_t* filter);

	};

	namespace Utility {

		void OpenExplorer(const std::string& path);
		void OpenFile(const std::string& path);
		void OpenWith(const std::string& path);

	}

	namespace Platform {

		bool Create_File(const std::string& path, bool createAllways = false);

	}

}
