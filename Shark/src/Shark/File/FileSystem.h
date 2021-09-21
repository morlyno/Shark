#pragma once

#undef CreateFile
#undef CreateDirectory
#undef DeleteFile

namespace Shark {

	using namespace std::string_literals;

}

namespace Shark::FileSystem {

	bool CreateFile(const std::string& path);
	bool CreateDirectory(const std::string& path);
	bool CreateDirectorys(const std::string& path);
	bool Rename(const std::string& oldPath, const std::string& newPath);
	bool Delete(const std::string& path);
	uint32_t DeleteAll(const std::string& path);
	//bool Exists(const std::string& path);

	bool CreateFile(const std::filesystem::path& path);
	bool CreateDirectory(const std::filesystem::path& path);
	bool CreateDirectorys(const std::filesystem::path& path);
	bool Rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
	bool Delete(const std::filesystem::path& path);
	uint32_t DeleteAll(const std::filesystem::path& path);
	bool Exists(const std::filesystem::path& path);
	std::string FileName(const std::filesystem::path& path);

}
