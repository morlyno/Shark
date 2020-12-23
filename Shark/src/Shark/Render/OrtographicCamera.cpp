#include "skpch.h"
#include "OrtographicCamera.h"

namespace Shark {

	OrtographicCamera::OrtographicCamera( float left,float right,float bottem,float top )
		:
		m_Position( 0.0f,0.0f,0.0f ),
		m_Projection( DirectX::XMMatrixOrthographicOffCenterLH( left,right,bottem,top,-10.0f,10.0f ) ),
		m_View( DirectX::XMMatrixTranslation( m_Position.x,m_Position.y,m_Position.z ) )
	{
		RecalculateViewProjection();
	}

	void OrtographicCamera::SetProjection( float left,float right,float bottem,float top )
	{
		m_Projection = DirectX::XMMatrixOrthographicOffCenterLH( left,right,bottem,top,-1.0f,1.0f );

		RecalculateViewProjection();
	}

	void OrtographicCamera::RecalculateViewProjection()
	{
		m_View = DirectX::XMMatrixTranslation( m_Position.x,m_Position.y,m_Position.z );

		m_ViewProjection = m_Projection * m_View;
	}

}
