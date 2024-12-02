#include "skpch.h"
#include "Platform/Windows/WindowsUtils.h"
#include "Shark/Utils/PlatformUtils.h"

#include "Shark/Core/Application.h"

#include "Shark/Utils/String.h"
#include "Shark/Utils/MemoryUtils.h"

#include <shellapi.h>
#include <ShlObj.h>
#include <comdef.h>

namespace Shark {

	namespace utils {

		LPCWSTR ExectueVerbToLPCWSTR(ExecuteVerb verb)
		{
			switch (verb)
			{
				case ExecuteVerb::Default: return nullptr;
				case ExecuteVerb::Edit: return L"edit";
				case ExecuteVerb::Explore: return L"explore";
				case ExecuteVerb::Run: return L"open";
				case ExecuteVerb::RunWith: return L"openas";
				case ExecuteVerb::Properties: return L"properties";
				case ExecuteVerb::RunAsAdmin: return L"runas";
			}

			SK_CORE_ASSERT(false, "Unkown ExecuteVerb");
			return nullptr;
		}

		static bool FileDialogShared(HWND parentWindow, bool save, bool multiSelect, const std::wstring& filter, uint32_t defaultFilterIndex, bool appenedFileExetention, const std::filesystem::path& defaultPath, bool overrideDefault, std::vector<std::filesystem::path>& out_MultiSelectResults)
		{
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
			return out_MultiSelectResults.size();
		}

		static IShellItem* GetRecycleBin()
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

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	/// WindowsUtils //////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string WindowsUtils::TranslateHResult(HRESULT hResult)
	{
		_com_error error(hResult);
		return String::ToNarrow(std::wstring_view(error.ErrorMessage()));
	}

	void WindowsUtils::SetThreadName(HANDLE thread, const std::wstring& name)
	{
		HRESULT hr = SetThreadDescription(thread, name.c_str());
		if (FAILED(hr))
		{
			_com_error error(hr);
			std::string msg = String::ToNarrow(std::wstring_view(error.ErrorMessage()));
			SK_CORE_ERROR_TAG("Windows", "Failed to set Thead Name! {}", msg);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Platform Implementation ///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void Platform::Initialize()
	{
	}

	void Platform::Shutdown()
	{
	}

	std::string_view Platform::GetPlatformName()
	{
		return "Windows"sv;
	}

	std::string_view Platform::GetConfiguration()
	{
#if SK_DEBUG
		return "Debug"sv;
#elif SK_RELEASE
		return "Release"sv;
#else
	#error Unkown Configuration
		return "Unkown Configuration"sv;
#endif
	}

	std::string_view Platform::GetArchitecture()
	{
		return "x64"sv;
	}

	float Platform::GetTime()
	{
		return (float)GetTicks() / GetTicksPerSecond();
	}

	uint64_t Platform::GetTicks()
	{
		LARGE_INTEGER largeInteger;
		QueryPerformanceCounter(&largeInteger);
		return largeInteger.QuadPart;
	}

	uint64_t Platform::GetTicksPerSecond()
	{
		LARGE_INTEGER largeInteger;
		QueryPerformanceFrequency(&largeInteger);
		return largeInteger.QuadPart;
	}

	glm::vec2 Platform::GetCursorPosition()
	{
		POINT point;
		GetCursorPos(&point);
		return { point.x, point.y };
	}

	bool Platform::IsKeyDown(KeyCode keyCode)
	{
		return GetAsyncKeyState((int)keyCode) & 0xff00;
	}

	bool Platform::IsMouseDown(MouseButton button)
	{
		return GetAsyncKeyState((int)button) & 0xff00;
	}

	void Platform::SetThreadName(std::thread& thread, const std::string& name)
	{
		std::wstring nameWide = String::ToWide(name);
		WindowsUtils::SetThreadName(thread.native_handle(), nameWide);
	}

	void Platform::SetThreadName(const std::string& name)
	{
		HANDLE thread = GetCurrentThread();
		std::wstring nameWide = String::ToWide(name);
		WindowsUtils::SetThreadName(thread, nameWide);
	}

	bool Platform::Execute(ExecuteVerb verb, const std::filesystem::path& executablePath, bool waitUntilFinished)
	{
		ExecuteSpecs specs;
		specs.Target = executablePath;
		specs.WaitUntilFinished = waitUntilFinished;
		specs.Verb = verb;
		return Execute(specs);
	}

	bool Platform::Execute(const ExecuteSpecs& specs)
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
			SK_CORE_ERROR_TAG("Windows", "ShellExectueExW Failed! {}", std::system_category().message(lastError));
			return false;
		}

		WaitForSingleObject(executeInfo.hProcess, INFINITE);
		CloseHandle(executeInfo.hProcess);
		return true;
	}

	bool Platform::CreateFile(const std::filesystem::path& filePath, bool createAllways)
	{
		std::string errorMsg;
		return Platform::CreateFile(filePath, createAllways, errorMsg);
	}

	bool Platform::CreateFile(const std::filesystem::path& filePath, bool createAllways, std::string& errorMsg)
	{
		HANDLE file = CreateFileW(filePath.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, createAllways ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			DWORD lasterror = GetLastError();
			auto message = std::system_category().message(lasterror);
			SK_CORE_ERROR_TAG("Windows", "Failed to create file! {0}", message);
			errorMsg = message;
			return false;
		}

		errorMsg.clear();
		CloseHandle(file);
		return true;
	}

	std::string Platform::GetEnvironmentVariable(const std::string& name)
	{
		DWORD bufferSize = GetEnvironmentVariableA(name.c_str(), nullptr, 0);
		if (bufferSize == 0)
		{
			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_ENVVAR_NOT_FOUND)
			{
				SK_CORE_ERROR_TAG("Windows", "Environment Variable not found");
				return std::string{};
			}
			auto msg = std::system_category().message(errorCode);
			SK_CORE_ERROR_TAG("Windows", "Failed to get EnvironmentVariable! {0}", msg);
			return std::string{};
		}

		std::string envVar;
		envVar.resize(bufferSize);

		DWORD result = GetEnvironmentVariableA(name.c_str(), envVar.data(), (DWORD)envVar.size());
		if (result == 0)
		{
			DWORD errorCode = GetLastError();
			auto msg = std::system_category().message(errorCode);
			SK_CORE_ERROR_TAG("Windows", "Failed to get EnvironmentVariable! {0}", msg);
			return std::string{};
		}

		envVar.erase(result);
		std::replace(envVar.begin(), envVar.end(), '\\', '/');
		return envVar;
	}

	bool Platform::SetEnvironmentVariable(const std::string& key, const std::string& value)
	{
		if (!SetEnvironmentVariableA(key.c_str(), value.c_str()))
		{
			DWORD errorCode = GetLastError();
			auto msg = std::system_category().message(errorCode);
			SK_CORE_ERROR_TAG("Windows", "Failed to set EnvironmentVariable! {0}", msg);
			return false;
		}

		return true;
	}

	std::filesystem::path Platform::OpenFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (utils::FileDialogShared((HWND)window.GetHandle(), false, false, filter, defaultFilterindex, false, defaultPath, overrideDefault, results))
			return results.back();
		return {};
	}

	std::filesystem::path Platform::SaveFileDialog(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault, bool appenedFileExetention)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (utils::FileDialogShared((HWND)window.GetHandle(), true, false, filter, defaultFilterindex, appenedFileExetention, defaultPath, overrideDefault, results))
			return results.back();
		return {};
	}

	std::vector<std::filesystem::path> Platform::OpenFileDialogMuliSelect(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::vector<std::filesystem::path> results;
		if (utils::FileDialogShared((HWND)window.GetHandle(), false, true, filter, defaultFilterindex, false, defaultPath, overrideDefault, results))
			return results;
		return {};
	}

	std::filesystem::path Platform::OpenDirectoryDialog(const std::filesystem::path& defaultPath)
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


	std::filesystem::path Platform::SaveDirectoryDialog(const std::filesystem::path& defaultPath)
	{
		std::filesystem::path result;

		IFileSaveDialog* fileDialog;
		if (SUCCEEDED(::CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void**)&fileDialog)))
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
					defualtPathItem->Release();
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

	std::vector<std::filesystem::path> Platform::OpenDirectoryDialogMultiSelect(const std::filesystem::path& defaultPath)
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

	bool Platform::MoveFileToRecycleBin(const std::filesystem::path& file)
	{
		IShellItem* recycleBin = utils::GetRecycleBin();
		if (!recycleBin)
			return false;

		HRESULT hr;
		IShellItem* fileItem = nullptr;
		std::error_code errorCode;
		std::filesystem::path windowsFilePath = std::filesystem::canonical(file, errorCode);
		if (errorCode)
		{
			SK_CORE_ERROR_TAG("Windows", "getting the canonical path failed!");
			SK_CORE_ERROR_TAG("Windows", "Reason: {0}", errorCode.message());
			return false;
		}

		hr = SHCreateItemFromParsingName(windowsFilePath.c_str(), NULL, IID_PPV_ARGS(&fileItem));
		if (hr != S_OK)
		{
			_com_error error(hr);
			std::string msg = String::ToNarrow(std::wstring_view(error.ErrorMessage()));
			SK_CORE_ERROR_TAG("Windows", msg);
		}

		bool success = false;
		if (SUCCEEDED(hr))
		{
			ITransferSource* transferSource = nullptr;
			if (SUCCEEDED(recycleBin->BindToHandler(NULL, BHID_Transfer, IID_PPV_ARGS(&transferSource))))
			{
				IShellItem* resultItem = nullptr;
				success = SUCCEEDED(transferSource->RecycleItem(fileItem, recycleBin, 0, &resultItem));
				transferSource->Release();
			}
			fileItem->Release();
		}
		recycleBin->Release();
		return success;
	}

}
