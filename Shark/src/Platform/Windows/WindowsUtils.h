#pragma once

#include <filesystem>

#undef CreateFile
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable

namespace Shark {

	enum class ExectueVerb
	{
		Default,
		Edit,
		Explore,
		Open,
		Properties,
		RunAsAdmin
	};

	struct ExecuteSpecs
	{
		std::filesystem::path Target;
		std::filesystem::path WorkingDirectory;
		std::wstring Params;
		ExectueVerb Verb = ExectueVerb::Default;
		bool WaitUntilFinished = false;
		bool InterhitConsole = true;
	};


	class WindowsUtils
	{
	public:
		static std::string TranslateLastError();
		static std::string TranslateErrorCode(DWORD error);
		static void SetThreadName(HANDLE thread, const std::wstring& name);
		static void SetThreadName(std::thread& thread, const std::wstring& name) { SetThreadName(thread.native_handle(), name); }
		static void SetThreadName(const std::wstring& name) { SetThreadName(GetCurrentThread(), name); }

		static bool Execute(ExectueVerb verb, const std::filesystem::path& executablePath, bool waitUntilFinished = false);
		static bool Execute(const ExecuteSpecs& specs);

		static bool RunProjectSetupSilent();

		static bool OpenExplorer(const std::filesystem::path& directory);
		static bool OpenFile(const std::filesystem::path& file);
		static bool OpenFileWith(const std::filesystem::path& file);

		static bool CreateFile(const std::filesystem::path& filePath, bool createAllways);

		static std::string GetEnvironmentVariable(const std::string& name);
		static bool SetEnvironmentVariable(const std::string& name, const std::string& value);

		static std::filesystem::path OpenFileDialog(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false);
		static std::filesystem::path SaveFileDialog(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false, bool appenedFileExetention = true);

		static std::filesystem::path OpenDirectoryDialog(const std::filesystem::path& defaultPath = std::filesystem::path{});

	private:
		static bool FileDialogShared(HWND parentWindow, bool save, const std::wstring& filter, uint32_t defaultFilterIndex, bool appenedFileExetention, const std::filesystem::path& defaultPath, bool overrideDefault, std::filesystem::path& out_Result);
	};

}
