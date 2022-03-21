#include "skpch.h"
#include "WindowsUtility.h"

namespace Shark {

	std::string TranslateErrorCode(DWORD error)
	{
		LPSTR messageBuffer = NULL;
		DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, messageBuffer, 0, NULL);

		auto message = std::string(messageBuffer, size);

		LocalFree(messageBuffer);

		return message;
	}

}
