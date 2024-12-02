#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <string>

#undef CreateFile
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable

namespace Shark {

#if SK_PLATFORM_WINDOWS
	// Used as a Platform agnostic handle
	using NativeHandle = HANDLE;
#endif

	enum class ExecuteVerb
	{
		Default,
		Edit,
		Explore,
		Run,
		RunWith,
		Properties,
		RunAsAdmin
	};

	struct ExecuteSpecs
	{
		std::filesystem::path Target;
		std::filesystem::path WorkingDirectory;
		std::wstring Params;
		ExecuteVerb Verb = ExecuteVerb::Default;
		bool WaitUntilFinished = false;
		bool InterhitConsole = true;
	};

	class Platform
	{
	public:
		static void Initialize();
		static void Shutdown();

		static std::string_view GetPlatformName();
		static std::string_view GetConfiguration();
		static std::string_view GetArchitecture();

		static float GetTime();
		static uint64_t GetTicks();
		static uint64_t GetTicksPerSecond();

		static glm::vec2 GetCursorPosition();
		static bool IsKeyDown(KeyCode keyCode);
		static bool IsMouseDown(MouseButton button);

		static void SetThreadName(std::thread& thread, const std::string& name);
		static void SetThreadName(const std::string& name);

		static bool Execute(ExecuteVerb verb, const std::filesystem::path& executablePath, bool waitUntilFinished = false);
		static bool Execute(const ExecuteSpecs& specs);

		static bool CreateFile(const std::filesystem::path& filePath, bool createAllways);
		static bool CreateFile(const std::filesystem::path& filePath, bool createAllways, std::string& errorMsg);

		static std::string GetEnvironmentVariable(const std::string& name);
		static bool SetEnvironmentVariable(const std::string& key, const std::string& value);

		static std::filesystem::path OpenFileDialog(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false);
		static std::filesystem::path SaveFileDialog(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false, bool appenedFileExetention = true);

		static std::vector<std::filesystem::path> OpenFileDialogMuliSelect(const std::wstring& filter, uint32_t defaultFilterindex = 1, const std::filesystem::path& defaultPath = {}, bool overrideDefault = false);

		static std::filesystem::path OpenDirectoryDialog(const std::filesystem::path& defaultPath = std::filesystem::path{});
		static std::filesystem::path SaveDirectoryDialog(const std::filesystem::path& defaultPath = std::filesystem::path{});
		static std::vector<std::filesystem::path> OpenDirectoryDialogMultiSelect(const std::filesystem::path& defaultPath = std::filesystem::path{});

		static bool MoveFileToRecycleBin(const std::filesystem::path& file);
	};

}
