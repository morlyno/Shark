#include "skpch.h"
#include "Platform/Windows/WindowsUtils.h"

#include "Shark/Utils/String.h"
#include "Shark/Utils/MemoryUtils.h"

#include "Shark/Core/Application.h"

#include <shellapi.h>
#include <ShlObj.h>

namespace Shark {

	namespace utils {

		LPCWSTR ExectueVerbToLPCWSTR(ExectueVerb verb)
		{
			switch (verb)
			{
				case ExectueVerb::Default: return nullptr;
				case ExectueVerb::Edit: return L"edit";
				case ExectueVerb::Explore: return L"explore";
				case ExectueVerb::Open: return L"open";
				case ExectueVerb::Properties: return L"properties";
				case ExectueVerb::RunAsAdmin: return L"runas";
			}

			SK_CORE_ASSERT(false, "Unkown ExecuteVerb");
			return nullptr;
		}

		void DefaultErrorHandle(DWORD errorCode)
		{
			auto message = WindowsUtils::TranslateErrorCode(errorCode);
			SK_CORE_ERROR("[Win32] {0}", message);
		}

	}

	std::string WindowsUtils::TranslateErrorCode(DWORD error)
	{
		LPSTR messageBuffer = NULL;
		DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, messageBuffer, 0, NULL);

		auto message = std::string(messageBuffer, size);

		LocalFree(messageBuffer);

		return message;
	}

	void WindowsUtils::SetThreadName(HANDLE thread, const std::wstring& name)
	{
		HRESULT hr = SetThreadDescription(thread, name.c_str());
		if (FAILED(hr))
		{
			DWORD code = HRESULT_CODE(hr);
			SK_CORE_ERROR("Failed to set Thead Name! {}", WindowsUtils::TranslateErrorCode(code));
		}
	}

	bool WindowsUtils::Execute(ExectueVerb verb, const std::filesystem::path& executablePath, bool waitUntilFinished)
	{
		ExecuteSpecs specs;
		specs.Target = executablePath;
		specs.WaitUntilFinished = waitUntilFinished;
		specs.Verb = verb;
		return Execute(specs);
	}

	bool WindowsUtils::Execute(const ExecuteSpecs& specs)
	{
		if (!std::filesystem::exists(specs.Target))
			return false;

		SHELLEXECUTEINFOW executeInfo;
		MemoryUtils::ZeroMemory(executeInfo);
		executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
		executeInfo.lpVerb = utils::ExectueVerbToLPCWSTR(specs.Verb);
		executeInfo.lpFile = specs.Target.c_str();
		executeInfo.lpParameters = specs.Params.c_str();
		executeInfo.lpDirectory = specs.WorkingDirectory.c_str();
		executeInfo.nShow = SW_SHOWDEFAULT;

		if (specs.WaitUntilFinished)
			executeInfo.fMask |= SEE_MASK_NOCLOSEPROCESS;

		if (specs.InterhitConsole)
			executeInfo.fMask |= SEE_MASK_NO_CONSOLE;

		BOOL succeded = ShellExecuteExW(&executeInfo);
		if (!succeded)
		{
			DWORD lastError = GetLastError();
			SK_CORE_ERROR("ShellExectueExW Failed! {}", WindowsUtils::TranslateErrorCode(lastError));
			return false;
		}

		WaitForSingleObject(executeInfo.hProcess, INFINITE);
		CloseHandle(executeInfo.hProcess);
		return true;
	}

	bool WindowsUtils::RunProjectSetupSilent()
	{
		SHELLEXECUTEINFOW executeInfo;
		MemoryUtils::ZeroMemory(executeInfo);
		executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
		executeInfo.lpVerb = L"open";

		std::filesystem::path premakeExe = Project::Directory() / "Premake/premake5.exe";
		executeInfo.lpFile = premakeExe.c_str();
		executeInfo.lpDirectory = Project::Directory().c_str();

		std::filesystem::path scriptPath = fmt::format("\"{}/premake5.lua\"", Project::Directory());
		executeInfo.lpParameters = scriptPath.c_str();
		executeInfo.nShow = SW_HIDE;
		executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

		BOOL succeded = ShellExecuteExW(&executeInfo);
		if (!succeded)
		{
			DWORD lastError = GetLastError();
			SK_CORE_ERROR("ShellExectueExW Failed! {}", WindowsUtils::TranslateErrorCode(lastError));
			return false;
		}

		WaitForSingleObject(executeInfo.hProcess, INFINITE);
		CloseHandle(executeInfo.hProcess);
		return true;
	}

	bool WindowsUtils::OpenExplorer(const std::filesystem::path& directory)
	{
		SK_CORE_ASSERT(std::filesystem::is_directory(directory));

		ExecuteSpecs specs;
		specs.Target = directory;
		specs.Verb = ExectueVerb::Explore;
		return Execute(specs);
	}

	bool WindowsUtils::OpenFile(const std::filesystem::path& file)
	{
		SK_CORE_ASSERT(std::filesystem::is_regular_file(file));

		ExecuteSpecs specs;
		specs.Target = file;
		specs.Verb = ExectueVerb::Open;
		return Execute(specs);
	}

	bool WindowsUtils::OpenFileWith(const std::filesystem::path& file)
	{
		std::filesystem::path exeFile = WindowsUtils::OpenFileDialog(L"exe|*.exe");
		SK_CORE_ASSERT(std::filesystem::exists(exeFile));
		if (!exeFile.empty())
		{
			ExecuteSpecs specs;
			specs.Target = exeFile;
			specs.Params = fmt::format(L"\"{}\"", file.native());
			return Execute(specs);
		}
		return false;
	}


	bool WindowsUtils::CreateFile(const std::filesystem::path& filePath, bool createAllways)
	{
		HANDLE file = CreateFileW(filePath.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, createAllways ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			DWORD lasterror = GetLastError();
			auto message = TranslateErrorCode(lasterror);
			SK_CORE_ERROR("[Win32] {0}", message);
			return false;
		}

		CloseHandle(file);
		return true;
	}

	std::string WindowsUtils::GetEnvironmentVariable(const std::string& name)
	{
		DWORD bufferSize = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
		if (bufferSize == 0)
		{
			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_ENVVAR_NOT_FOUND)
			{
				SK_CORE_ERROR("[Win32] Environment Variable not found");
				return std::string{};
			}
			utils::DefaultErrorHandle(errorCode);
			return std::string{};
		}

		std::string envVar;
		envVar.resize(bufferSize);

		DWORD result = GetEnvironmentVariableA(name.c_str(), envVar.data(), envVar.size());
		if (result == 0)
		{
			DWORD errorCode = GetLastError();
			utils::DefaultErrorHandle(errorCode);
			return std::string{};
		}

		envVar.erase(result);
		std::replace(envVar.begin(), envVar.end(), '\\', '/');
		return envVar;
	}

	std::filesystem::path WindowsUtils::OpenFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::filesystem::path result;
		if (FileDialogShared((HWND)window.GetHandle(), false, filter, defaultFilterindex, false, defaultPath, overrideDefault, result))
			return result;
		return {};
	}

	std::filesystem::path WindowsUtils::SaveFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault, bool appenedFileExetention)
	{
		auto& window = Application::Get().GetWindow();
		std::filesystem::path result;
		if (FileDialogShared((HWND)window.GetHandle(), true, filter, defaultFilterindex, appenedFileExetention, defaultPath, overrideDefault, result))
			return result;
		return {};
	}

	std::filesystem::path WindowsUtils::OpenDirectoryDialog(const std::filesystem::path& defaultPath)
	{
		std::filesystem::path result;

		IFileOpenDialog* fileDialog;
		if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void**)&fileDialog)))
		{
			DWORD flags;
			fileDialog->GetOptions(&flags);
			fileDialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_DONTADDTORECENT);

			if (!defaultPath.empty())
			{
				auto windowsDefaultPath = std::filesystem::absolute(defaultPath);
				windowsDefaultPath = String::FormatWindowsCopy(windowsDefaultPath);
				IShellItem* defualtPathItem;
				if (SUCCEEDED(::SHCreateItemFromParsingName(windowsDefaultPath.c_str(), NULL, IID_PPV_ARGS(&defualtPathItem))))
				{
					fileDialog->SetDefaultFolder(defualtPathItem);
				}
			}

			auto& window = Application::Get().GetWindow();
			if (SUCCEEDED(fileDialog->Show((HWND)window.GetHandle())))
			{
				IShellItem* shellItem;
				if (SUCCEEDED(fileDialog->GetResult(&shellItem)))
				{
					PWSTR filePath;
					if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
					{
						result = filePath;
						CoTaskMemFree(filePath);
					}

					shellItem->Release();
				}

				fileDialog->Release();
			}
		}

		return result;
	}

	bool WindowsUtils::FileDialogShared(HWND parentWindow, bool save, const std::wstring& filter, uint32_t defaultFilterIndex, bool appenedFileExetention, const std::filesystem::path& defaultPath, bool overrideDefault, std::filesystem::path& out_Result)
	{
		bool success = false;

		// TODO(moro): attach DataType/File extention to out_Result (only necessary for FileSaveDialog)

		IFileDialog* fileDialog;
		if (SUCCEEDED(::CoCreateInstance(save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, save ? IID_IFileSaveDialog : IID_IFileOpenDialog, (void**)&fileDialog)))
		{
			DWORD flags;
			fileDialog->GetOptions(&flags);
			flags |= FOS_DONTADDTORECENT | FOS_FILEMUSTEXIST;
			fileDialog->SetOptions(flags);

			if (!defaultPath.empty())
			{
				auto windowsDefaultPath = std::filesystem::absolute(defaultPath);
				String::FormatWindows(windowsDefaultPath);
				IShellItem* defualtPathItem;
				if (SUCCEEDED(::SHCreateItemFromParsingName(windowsDefaultPath.c_str(), NULL, IID_PPV_ARGS(&defualtPathItem))))
				{
					if (overrideDefault)
						fileDialog->SetFolder(defualtPathItem);
					else
						fileDialog->SetDefaultFolder(defualtPathItem);
				}
			}

			if (!filter.empty())
			{
				// Filter format name0|filter0|name1|filter1
				std::vector<std::wstring> unformatedFilters;
				String::SplitString(filter, L"|", unformatedFilters);

				std::vector<COMDLG_FILTERSPEC> fileDialogFilters;
				if (unformatedFilters.size() % 2 == 0)
				{
					fileDialogFilters.reserve(unformatedFilters.size() / 2);
					for (uint32_t i = 0; i < unformatedFilters.size();)
					{
						COMDLG_FILTERSPEC& filterSpec = fileDialogFilters.emplace_back();
						filterSpec.pszName = unformatedFilters[i++].c_str();
						filterSpec.pszSpec = unformatedFilters[i++].c_str();
					}
				}

				fileDialog->SetFileTypes((UINT)fileDialogFilters.size(), fileDialogFilters.data());
				fileDialog->SetFileTypeIndex(defaultFilterIndex);

				if (save && appenedFileExetention)
				{
					uint32_t index = defaultFilterIndex ? (defaultFilterIndex * 2) - 1 : 1;
					std::wstring defaultFilter = unformatedFilters[index];
					if (defaultFilter.front() == L'.')
						defaultFilter.erase(0);

					fileDialog->SetDefaultExtension(defaultFilter.c_str());
				}

			}

			if (SUCCEEDED(fileDialog->Show(parentWindow)))
			{
				IShellItem* shellItem;
				if (SUCCEEDED(fileDialog->GetResult(&shellItem)))
				{
					PWSTR filePath;
					if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
					{
						out_Result = filePath;
						String::FormatDefault(out_Result);

						CoTaskMemFree(filePath);
					}

					shellItem->Release();
				}

				fileDialog->Release();
			}
		}

		return true;
	}

}