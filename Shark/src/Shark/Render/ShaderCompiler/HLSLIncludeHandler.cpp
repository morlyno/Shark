#include "skpch.h"
#include "HLSLIncludeHandler.h"

#include "Shark/Render/ShaderCompiler/ShaderPreprocessor.h"
#include "Shark/Utils/String.h"

namespace Shark {

	HLSLIncludeHandler::HLSLIncludeHandler(ShaderPreprocessor* preprocessor, ATL::CComPtr<IDxcUtils> utils)
		: m_Preprocessor(preprocessor), m_Utils(utils)
	{
	}

	HRESULT STDMETHODCALLTYPE HLSLIncludeHandler::LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource)
	{
		std::wstring_view filename = pFilename;
		if (filename.starts_with(L"./") || filename.starts_with(L".\\"))
		{
			filename.remove_prefix(2);
		}

		std::filesystem::path file = filename;

		const auto i = std::ranges::find(m_Preprocessor->Includes, file, &IncludeData::OriginalPath);
		if (i == m_Preprocessor->Includes.end())
			return S_FALSE;

		const auto& includeData = *i;

		IDxcBlobEncoding* encoding;
		m_Utils->CreateBlob(includeData.Source.c_str(), static_cast<UINT32>(includeData.Source.size()), CP_UTF8, &encoding);

		*ppIncludeSource = encoding;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE HLSLIncludeHandler::QueryInterface(REFIID riid, void** ppvObject)
	{
		return S_FALSE;
	}

	ULONG STDMETHODCALLTYPE HLSLIncludeHandler::AddRef(void)
	{
		return 0;
	}

	ULONG STDMETHODCALLTYPE HLSLIncludeHandler::Release(void)
	{
		return 0;
	}

}
