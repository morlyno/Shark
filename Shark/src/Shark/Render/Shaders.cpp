#include "skpch.h"
#include "Shaders.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	std::shared_ptr<Shaders> Shaders::Create( const std::string& vertexshaderSrc,const std::string& pixelshaderSrc )
	{
		switch ( RendererAPI::GetAPI() )
		{
			case RendererAPI::API::None: SK_CORE_ASSERT( false,"RendererAPI not specified" ); return nullptr;
			case RendererAPI::API::DirectX11: return std::make_shared<DirectXShaders>( vertexshaderSrc,pixelshaderSrc );
		}
		SK_CORE_ASSERT( false,"Unknown RendererAPI" );
		return nullptr;
	}
	
	std::shared_ptr<Shaders> Shaders::Create( VertexLayout& layout,const std::string& vertexshaderSrc,const std::string& pixelshaderSrc )
	{
		switch ( RendererAPI::GetAPI() )
		{
			case RendererAPI::API::None: SK_CORE_ASSERT( false,"RendererAPI not specified" ); return nullptr;
			case RendererAPI::API::DirectX11: return std::make_shared<DirectXShaders>( layout,vertexshaderSrc,pixelshaderSrc );
		}
		SK_CORE_ASSERT( false,"Unknown RendererAPI" );
		return nullptr;
	}

}