#include "skpch.h"
#include "Shark/Utility/PlatformUtils.h"

#include "Shark/Core/Application.h"

#include "Platform/Windows/WindowsUtility.h"

namespace Shark {

	namespace FileDialogs {

		std::string OpenFile(const char* filter)
		{
			OPENFILENAMEA ofn;
			CHAR szFile[260] = { 0 };
			CHAR currentDir[256] = { 0 };
			ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
			ofn.lStructSize = sizeof(OPENFILENAMEA);
			ofn.hwndOwner = (HWND)Application::Get().GetWindow().GetHandle();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			if (GetCurrentDirectoryA(256, currentDir))
				ofn.lpstrInitialDir = currentDir;
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;

			if (GetOpenFileNameA(&ofn) == TRUE)
				return ofn.lpstrFile;
			return std::string{};
		}

		std::string SaveFile(const char* filter)
		{
			OPENFILENAMEA ofn;
			CHAR szFile[260] = { 0 };
			CHAR currentDir[256] = { 0 };
			ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
			ofn.lStructSize = sizeof(OPENFILENAMEA);
			ofn.hwndOwner = (HWND)Application::Get().GetWindow().GetHandle();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			if (GetCurrentDirectoryA(256, currentDir))
				ofn.lpstrInitialDir = currentDir;
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;

			// Sets the default extension by extracting it from the filter
			ofn.lpstrDefExt = strchr(filter, '\0') + 1;

			if (GetSaveFileNameA(&ofn) == TRUE)
				return ofn.lpstrFile;
			return std::string{};
		}

	}

	namespace Utility {

		void OpenExplorer(const std::string& path)
		{
			auto&& cmd = "explorer " + path;
			system(cmd.c_str());
		}

		void OpenFile(const std::string& path)
		{
			auto&& cmd = "start " + path;
			system(cmd.c_str());
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