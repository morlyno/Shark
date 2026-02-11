#pragma once

#include <dxc/dxcapi.h>
#include <nvrhi/nvrhi.h>

namespace Shark {

	class ShaderPreprocessor;

	class HLSLIncludeHandler : public IDxcIncludeHandler
	{
	public:
		HLSLIncludeHandler(ShaderPreprocessor* preprocessor, nvrhi::RefCountPtr<IDxcUtils> utils);

		virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override;


		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		virtual ULONG STDMETHODCALLTYPE AddRef(void);
		virtual ULONG STDMETHODCALLTYPE Release(void);

	private:
		ShaderPreprocessor* m_Preprocessor;
		nvrhi::RefCountPtr<IDxcUtils> m_Utils;
	};

}
