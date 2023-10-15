#pragma once

namespace Shark {

	class WindowsUtils
	{
	public:
		static std::string TranslateHResult(HRESULT hResult);
		static void SetThreadName(HANDLE thread, const std::wstring& name);
	};

}
