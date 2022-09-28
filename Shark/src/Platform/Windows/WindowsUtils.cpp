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

	static void CheckHResult(HRESULT hr)
	{
		if (hr != S_OK)
		{
			std::string msg = std::system_category().message(hr);
			SK_CORE_ERROR("[Win32] {0}", msg);
		}
	}

	static void LogHResult(HRESULT hr)
	{
		std::string msg = std::system_category().message(hr);
		SK_CORE_ERROR("[Win32] {0}", msg);
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

		std::filesystem::path premakeExe = Project::GetDirectory() / "Premake/premake5.exe";
		executeInfo.lpFile = premakeExe.c_str();
		executeInfo.lpDirectory = Project::GetDirectory().c_str();

		std::filesystem::path scriptPath = fmt::format("\"{}/premake5.lua\"", Project::GetDirectory());
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

		DWORD result = GetEnvironmentVariableA(name.c_str(), envVar.data(), (DWORD)envVar.size());
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

	bool WindowsUtils::SetEnvironmentVariable(const std::string& key, const std::string& value)
	{
		if (!SetEnvironmentVariableA(key.c_str(), value.c_str()))
		{
			DWORD errorCode = GetLastError();
			utils::DefaultErrorHandle(errorCode);
			return false;
		}

		return true;
	}

	std::filesystem::path WindowsUtils::OpenFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (FileDialogShared((HWND)window.GetHandle(), false, false, filter, defaultFilterindex, false, defaultPath, overrideDefault, results))
			return results.back();
		return {};
	}

	std::filesystem::path WindowsUtils::SaveFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault, bool appenedFileExetention)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (FileDialogShared((HWND)window.GetHandle(), true, false, filter, defaultFilterindex, appenedFileExetention, defaultPath, overrideDefault, results))
			return results.back();
		return {};
	}

	std::vector<std::filesystem::path> WindowsUtils::OpenFileDialogMuliSelect(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (FileDialogShared((HWND)window.GetHandle(), false, true, filter, defaultFilterindex, false, defaultPath, overrideDefault, results))
			return results;
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

			}
			fileDialog->Release();
		}

		return result;
	}

	std::vector<std::filesystem::path> WindowsUtils::OpenDirectoryDialogMultiSelect(const std::filesystem::path& defaultPath)
	{
		std::vector<std::filesystem::path> result;

		IFileOpenDialog* fileDialog;
		if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void**)&fileDialog)))
		{
			DWORD flags;
			fileDialog->GetOptions(&flags);
			fileDialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_DONTADDTORECENT | FOS_ALLOWMULTISELECT);

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
				IShellItemArray* shellItemArray;
				if (SUCCEEDED(fileDialog->GetResults(&shellItemArray)))
				{
					DWORD count = 0;
					shellItemArray->GetCount(&count);
					for (uint32_t i = 0; i < count; i++)
					{
						IShellItem* shellItem;
						if (SUCCEEDED(shellItemArray->GetItemAt(i, &shellItem)))
						{
							PWSTR filePath;
							if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
							{
								result.emplace_back(String::FormatDefaultCopy(filePath));
								CoTaskMemFree(filePath);
							}
							shellItem->Release();
						}
					}
					shellItemArray->Release();
				}

			}
			fileDialog->Release();
		}

		return result;
	}

	void WindowsUtils::MoveFileToRecycleBin(const std::filesystem::path& file)
	{
		IShellItem* recycleBin = GetRecycleBin();
		if (!recycleBin)
			return;

		HRESULT hr;
		IShellItem* fileItem = nullptr;
		std::error_code errorCode;
		std::filesystem::path windowsFilePath = std::filesystem::canonical(file, errorCode);
		if (errorCode)
		{
			SK_CORE_ERROR("[WindowsUtils] getting the canonical path failed!");
			SK_CORE_ERROR("[WindowsUtils] Reason: {0}", errorCode.message());
			return;
		}

		hr = SHCreateItemFromParsingName(windowsFilePath.c_str(), NULL, IID_PPV_ARGS(&fileItem));
		CheckHResult(hr);

		if (SUCCEEDED(hr))
		{
			ITransferSource* transferSource = nullptr;
			if (SUCCEEDED(recycleBin->BindToHandler(NULL, BHID_Transfer, IID_PPV_ARGS(&transferSource))))
			{
				IShellItem* resultItem = nullptr;
				transferSource->RecycleItem(fileItem, recycleBin, 0, &resultItem);
				transferSource->Release();
			}
			fileItem->Release();
		}
		recycleBin->Release();
	}

	IShellItem* WindowsUtils::GetRecycleBin()
	{
		IShellItem* recycleBin = nullptr;

		IKnownFolderManager* knownFolderManager = nullptr;
		if (SUCCEEDED(::CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_IKnownFolderManager, (void**)&knownFolderManager)))
		{
			IKnownFolder* recycleBinFolder = nullptr;
			if (SUCCEEDED(knownFolderManager->GetFolder(FOLDERID_RecycleBinFolder, &recycleBinFolder)))
			{
				recycleBinFolder->GetShellItem(0, IID_IShellItem, (void**)&recycleBin);
				recycleBinFolder->Release();
			}

			knownFolderManager->Release();
		}

		return recycleBin;
	}

	bool WindowsUtils::FileDialogShared(HWND parentWindow, bool save, bool multiSelect, const std::wstring& filter, uint32_t defaultFilterIndex, bool appenedFileExetention, const std::filesystem::path& defaultPath, bool overrideDefault, std::vector<std::filesystem::path>& out_MultiSelectResults)
	{
		bool success = false;

		// maby allready done, not 100% sure
		// TODO(moro): attach DataType/File extention to out_Result (only necessary for FileSaveDialog)

		IFileDialog* fileDialog;
		if (SUCCEEDED(::CoCreateInstance(save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, save ? IID_IFileSaveDialog : IID_IFileOpenDialog, (void**)&fileDialog)))
		{
			DWORD flags;
			fileDialog->GetOptions(&flags);
			flags |= FOS_DONTADDTORECENT | FOS_FILEMUSTEXIST;

			if (multiSelect)
				flags |= FOS_ALLOWMULTISELECT;

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
				if (multiSelect)
				{
					IFileOpenDialog* fileOpendDialog = nullptr;
					if (SUCCEEDED(fileDialog->QueryInterface(IID_IFileOpenDialog, (void**)&fileOpendDialog)))
					{
						IShellItemArray* results = nullptr;
						if (SUCCEEDED(fileOpendDialog->GetResults(&results)))
						{
							DWORD count = 0;
							results->GetCount(&count);
							out_MultiSelectResults.reserve(count);
							for (DWORD i = 0; i < count; i++)
							{
								IShellItem* shellItem = nullptr;
								if (SUCCEEDED(results->GetItemAt(i, &shellItem)))
								{
									PWSTR filePath;
									if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
									{
										out_MultiSelectResults.emplace_back(String::FormatDefaultCopy(filePath));
										CoTaskMemFree(filePath);
									}
									shellItem->Release();
								}
							}
							results->Release();
						}
						fileOpendDialog->Release();
					}
				}
				else
				{
					IShellItem* shellItem;
					if (SUCCEEDED(fileDialog->GetResult(&shellItem)))
					{
						PWSTR filePath;
						if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
						{
							out_MultiSelectResults.emplace_back(String::FormatDefaultCopy(filePath));
							CoTaskMemFree(filePath);
						}

						shellItem->Release();
					}

					fileDialog->Release();
				}
			}
		}
		return true;
	}

}