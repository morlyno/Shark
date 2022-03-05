#include "skpch.h"
#include "Shark/Utility/PlatformUtils.h"

#include "Shark/Utility/String.h"
#include "Shark/Core/Application.h"
#include "Platform/Windows/WindowsUtility.h"

#include "Shark/File/FileSystem.h"

#include <shellapi.h>
#include <ShlObj.h>

namespace Shark {

	std::filesystem::path FileDialogs::OpenFile(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::filesystem::path result;
		if (FileDialogShared(window.GetHandle(), false, filter, defaultFilterindex, defaultPath, overrideDefault, result))
			return result;
		return {};
	}

	std::filesystem::path FileDialogs::SaveFile(const std::wstring& filter, uint32_t defaultFilterindex, const std::filesystem::path& defaultPath, bool overrideDefault)
	{
		auto& window = Application::Get().GetWindow();
		std::filesystem::path result;
		if (FileDialogShared(window.GetHandle(), true, filter, defaultFilterindex, defaultPath, overrideDefault, result))
			return result;
		return {};
	}

	std::filesystem::path FileDialogs::OpenDirectory(const std::filesystem::path& defaultPath)
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
				windowsDefaultPath = FileSystem::FormatWindowsCopy(windowsDefaultPath);
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

	bool FileDialogs::FileDialogShared(WindowHandle parentWindow, bool save, const std::wstring& filter, uint32_t defaultFilterIndex, const std::filesystem::path& defaultPath, bool overrideDefault, std::filesystem::path& out_Result)
	{
		bool success = false;

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
				windowsDefaultPath = FileSystem::FormatWindowsCopy(windowsDefaultPath);
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
					for (uint32_t i = 0; i < unformatedFilters.size();)
					{
						COMDLG_FILTERSPEC& filterSpec = fileDialogFilters.emplace_back();
						filterSpec.pszName = unformatedFilters[i++].c_str();
						filterSpec.pszSpec = unformatedFilters[i++].c_str();
					}
				}
				fileDialog->SetFileTypes((UINT)fileDialogFilters.size(), fileDialogFilters.data());
				fileDialog->SetFileTypeIndex(defaultFilterIndex);
			}

			if (SUCCEEDED(fileDialog->Show((HWND)parentWindow)))
			{
				IShellItem* shellItem;
				if (SUCCEEDED(fileDialog->GetResult(&shellItem)))
				{
					PWSTR filePath;
					if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath)))
					{
						out_Result = filePath;
						FileSystem::FormatDefault(out_Result);
						CoTaskMemFree(filePath);
					}

					shellItem->Release();
				}

				fileDialog->Release();
			}
		}

		return true;
	}

	namespace Utility {

		void OpenExplorer(const std::filesystem::path& directory)
		{
			SK_CORE_ASSERT(std::filesystem::is_directory(directory));

			SHELLEXECUTEINFOW executeInfo;
			ZeroMemory(&executeInfo, sizeof(SHELLEXECUTEINFOW));
			executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			executeInfo.fMask = SEE_MASK_ASYNCOK;
			executeInfo.lpVerb = L"open";
			executeInfo.lpFile = directory.c_str();
			executeInfo.nShow = SW_SHOWDEFAULT;
			ShellExecuteExW(&executeInfo);
		}

		void OpenFile(const std::filesystem::path& file)
		{
			SK_CORE_ASSERT(std::filesystem::is_regular_file(file));

			SHELLEXECUTEINFOW executeInfo;
			ZeroMemory(&executeInfo, sizeof(SHELLEXECUTEINFOW));
			executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
			executeInfo.fMask = SEE_MASK_ASYNCOK;
			executeInfo.lpVerb = L"open";
			executeInfo.lpFile = file.c_str();
			executeInfo.nShow = SW_SHOWDEFAULT;
			ShellExecuteExW(&executeInfo);
		}

		void OpenFileWith(const std::filesystem::path& file)
		{
			std::filesystem::path exeFile = FileDialogs::OpenFile(L"exe|*.exe");
			SK_CORE_ASSERT(std::filesystem::exists(exeFile));
			if (!exeFile.empty())
			{
				std::wstring params = fmt::format(L"\"{}\"", file.native());
				SHELLEXECUTEINFOW executeInfo;
				ZeroMemory(&executeInfo, sizeof(SHELLEXECUTEINFOW));
				executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
				executeInfo.fMask = SEE_MASK_ASYNCOK | SEE_MASK_CLASSNAME;
				executeInfo.lpVerb = L"open";
				executeInfo.lpFile = exeFile.c_str();
				executeInfo.lpParameters = params.c_str();
				executeInfo.nShow = SW_SHOWDEFAULT;
				executeInfo.lpClass = L".exe";
				ShellExecuteExW(&executeInfo);
			}

		}

	}

	namespace Platform {

		bool Platform::Create_File(const std::string& path, bool createAllways)
		{
			HANDLE file = CreateFileA(path.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, createAllways ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

#if SK_LOG_FILESYSTEM
			if (file == INVALID_HANDLE_VALUE)
			{
				DWORD lasterror = GetLastError();
				auto message = GetLastErrorMsg(lasterror);
				SK_CORE_ERROR(message);
			}
#endif

			CloseHandle(file);
			return true;
		}

	}

}