#pragma once

#include "Shark/Render/Topology.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

namespace Shark {

	class DirectXTopology : public Topology
	{
	public:
		DirectXTopology(TopologyMode topology);
		virtual ~DirectXTopology();

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void SetTopology(TopologyMode toplogy) override;
		virtual TopologyMode GetTopology() const override;
	private:
		WeakRef<DirectXRendererAPI> m_DXApi;
		D3D11_PRIMITIVE_TOPOLOGY m_D3DTopology;
	};

}
