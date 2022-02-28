#include "skpch.h"
#include "FileSystem.h"

#include <iostream>
#include <filesystem>

#include "Shark/Utility/PlatformUtils.h"

#if SK_LOG_FILESYSTEM
#define SK_FS_ERROR(...) SK_CORE_ERROR(__VA_ARGS__)
#define SK_FS_INFO(...) SK_CORE_INFO(__VA_ARGS__)
#else
#define SK_FS_ERROR(...) SK_CORE_ASSERT(false, fmt::format(__VA_ARGS__))
#define SK_FS_INFO(...)
#endif

namespace Shark::FileSystem {

	bool CreateFile(const std::filesystem::path& path)                        { return CreateFile(path.string()); }

	bool CreateDirectory(const std::string& path)                             { return CreateDirectory(std::filesystem::path(path)); }
	bool CreateDirectorys(const std::string& path)                            { return CreateDirectorys(std::filesystem::path(path)); }
	bool Rename(const std::string& oldPath, const std::string& newPath)       { return Rename(std::filesystem::path(oldPath), std::filesystem::path(newPath)); }
	bool Delete(const std::string& path)                                      { return Delete(std::filesystem::path(path)); }
	uint32_t DeleteAll(const std::string& path)                               { return DeleteAll(std::filesystem::path(path)); }
	//bool Exists(const std::string& path)                                      { return Exists(std::filesystem::path(path)); }


	bool CreateFile(const std::string& path)
	{
		if (Platform::Create_File(path))
		{ 
			SK_FS_INFO("File Created [{0}]", path);
			return true;
		}
		SK_FS_ERROR("Failed to Create file [{0}]", path);
		return false;
	}

	bool CreateDirectory(const std::filesystem::path& path)
	{
		std::error_code err;
		std::filesystem::create_directory(path, err);
		if (err)
		{
			SK_FS_ERROR(err.message());
			return false;
		}
		SK_FS_INFO("Directory Created [{0}]", path);
		return true;
	}

	bool CreateDirectorys(const std::filesystem::path& path)
	{
		std::error_code err;
		std::filesystem::create_directories(path, err);
		if (err)
		{
			SK_FS_ERROR(err.message());
			return false;
		}
		SK_FS_INFO("Directory Created [{0}]", path);
		return true;
	}

	bool Rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
	{
		std::error_code err;
		std::filesystem::rename(oldPath, newPath, err);
		if (err)
		{
			SK_FS_ERROR(err.message());
			return false;
		}
		SK_FS_INFO("Entry Renamed from [{0}] to [{1}]", oldPath, newPath);
		return true;
	}

	bool Delete(const std::filesystem::path& path)
	{
		std::error_code err;
		bool result = std::filesystem::remove(path, err);
		if (err)
		{
			SK_FS_ERROR(err.message());
			return result;
		}
		SK_FS_INFO("Deleted [{0}]", path);
		return result;
	}

	uint32_t DeleteAll(const std::filesystem::path& path)
	{
		std::error_code err;
		uint32_t deleted = (uint32_t)std::filesystem::remove_all(path, err);
		if (err)
		{
			SK_FS_ERROR(err.message());
			return deleted;
		}
		SK_FS_INFO("Deleted {0} in [{1}]", deleted, path);
		return deleted;
	}

	bool Exists(const std::filesystem::path& path)
	{
		return std::filesystem::exists(path);
	}

	std::string FileName(const std::filesystem::path& path)
	{
		return std::move(path.stem().string());
	}

	std::filesystem::path FormatWindowsCopy(const std::filesystem::path& path)
	{
		std::wstring str;
		std::replace(str.begin(), str.end(), L'/', L'\\');
		return str;
	}

	std::filesystem::path FormatDefaultCopy(const std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'\\', L'/');
		return str;
	}

	void FormatWindows(std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'/', L'\\');
		path = str;
	}

	void FormatDefault(std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'\\', L'/');
		path = str;
	}

	bool IsDefaultFormat(const std::filesystem::path& path)
	{
		const std::wstring& str = path;
		return str.find(L'\\') == std::wstring::npos;
	}

	bool ValidateSceneFilePath(const std::filesystem::path& sceneFilePath)
	{
		return !sceneFilePath.empty() && Exists(sceneFilePath) && (sceneFilePath.extension() == L".skscene");
	}

}