#pragma once

namespace Shark {

	class FileDialogs
	{
	public:
		static std::filesystem::path OpenFile(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false);
		static std::filesystem::path SaveFile(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false);

		static std::filesystem::path OpenDirectory(const std::filesystem::path& defaultPath = std::filesystem::path{});

	private:
		static bool FileDialogShared(WindowHandle parentWindow, bool save, const std::wstring& filter, uint32_t defaultFilterIndex, const std::filesystem::path& defaultPath, bool overrideDefault, std::filesystem::path& out_Result);
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
